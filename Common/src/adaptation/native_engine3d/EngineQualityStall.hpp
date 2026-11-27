// Tracks the collective quality debt at the existing round-head observation point.
#pragma once

#include "../../../include/adaptation/CNativeEngine3D.hpp"
#include "../../../include/adaptation/CNativeLocalCore3D.hpp"
#include "../../../include/adaptation/CNativeTimerTree3D.hpp"
#include "../../../include/parallelization/CPassiveComm.hpp"
#include <algorithm>
#include <array>
#include <iomanip>
#include <iostream>
#include <limits>

namespace SU2Native3D {
namespace quality_stall {

struct State {
  std::array<double, 4> ring{};
  size_t count = 0;
  double best = -1;
  bool request = false;
  bool used = false;

  void ResetPhase() {
    ring.fill(0);
    count = 0;
    best = -1;
    request = false;
  }
};

inline bool Enabled(bool close_queue, SU2NativeLocal3D::CollapseProbe probe) {
  using Probe = SU2NativeLocal3D::CollapseProbe;
  return close_queue && probe != Probe::FP3B && probe != Probe::FP3D;
}

inline bool Active(const State* stall, bool closed, bool improvement, bool repairing, bool metric_cleanup) {
  return stall && closed && improvement && repairing && !metric_cleanup && !stall->used;
}

// FP3-E compares the best debt with the best debt four observed windows earlier.
inline bool IsStalled(double best, double before, double bad) {
  return before >= 0 && bad > 0 && best > 0.95 * before;
}

struct UpdateResult {
  double before;
  bool stalled;
};

inline UpdateResult Update(State& stall, double debt, double bad, bool observed) {
  if (!observed) return {-1, false};
  stall.best = stall.best < 0 ? debt : std::min(stall.best, debt);
  const auto slot = stall.count % stall.ring.size();
  ++stall.count;
  const double before = stall.count > stall.ring.size() ? stall.ring[slot] : -1;
  const bool stalled = IsStalled(stall.best, before, bad);
  if (stalled) stall.request = stall.used = true;
  stall.ring[slot] = stall.best;
  return {before, stalled};
}

inline void Observe(State& stall, const StageProgress& p, CPassiveComm::Communicator comm, bool timings,
                    uint64_t round) {
  const bool valid = p.cache_consistent && p.census_complete && p.shape_complete;
  const double send[] = {double(p.q_hinge_squared + p.j_hinge_squared), double(p.q_bad_cells + p.j_bad_cells),
                         valid ? 0.0 : 1.0};
  double recv[3];
  {
    SU2NativeTiming3D::Scope communication_timer(SU2NativeTiming3D::Operation::Communication);
    CPassiveComm::Allreduce(send, recv, 3, CPassiveComm::Op::SUM, comm);
  }
  const bool observed = recv[2] == 0;
  const auto update = Update(stall, recv[0], recv[1], observed);
  if (timings) {
    const auto flags = std::cout.flags();
    const auto precision = std::cout.precision(std::numeric_limits<double>::max_digits10);
    std::cout << std::defaultfloat << "NATIVE3D_QUALITY_STALL round=" << round << " observed=" << observed
              << " debt=" << recv[0] << " bad=" << recv[1] << " best=" << stall.best << " best_before="
              << update.before << " count=" << stall.count << " stalled=" << update.stalled << '\n';
    std::cout.flags(flags);
    std::cout.precision(precision);
  }
}

struct HandoffResult {
  const char* by;
  const char* failed;
  bool taken;

  bool Taken() const { return taken; }
};

template <class Improvement, class Demand, class Ordinary>
HandoffResult EvaluateHandoff(bool metric_cleanup, bool improvement_phase, Improvement improvement, Demand demand,
                              Ordinary ordinary, bool any, bool request) {
  if (metric_cleanup) return {"none", "metric_cleanup", false};
  if (!improvement_phase) return {"none", "phase", false};
  if (!improvement()) return {"none", "improvement", false};
  if (!demand()) return {"none", "demand", false};
  if (ordinary()) return {"ordinary", "none", true};
  if (!any) return {"no_candidate", "none", true};
  if (request) return {"stall", "none", true};
  return {"none", "none", false};
}

inline void PrintHandoff(uint64_t round, const HandoffResult& decision) {
  std::cout << "NATIVE3D_QUALITY_STALL_HANDOFF round=" << round << " taken=" << decision.Taken()
            << " by=" << decision.by << " failed=" << decision.failed << '\n';
}

}  // namespace quality_stall
}  // namespace SU2Native3D
