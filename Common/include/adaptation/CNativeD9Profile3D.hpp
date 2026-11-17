/* Diagnostic-only D9 observations. Owns fixed thread-local counters, never mesh state,
 * geometry queries, admission, collectives or acceptance decisions. */
#pragma once
#include "CNativeTimerTree3D.hpp"
#include <array>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>

namespace SU2NativeD9 {
using Clock = std::chrono::steady_clock;
enum class Site { Guidance, SmoothBoundary, ShellBoundary, CoupledBoundary, Census, Finish, Count };
enum class Work { Other, Export, Projection, Move, Guidance, Count };
constexpr size_t SiteCount = size_t(Site::Count), WorkCount = size_t(Work::Count);
struct Queries {
  uint64_t scopes = 0, direct = 0, upper = 0, classify = 0, magnitude = 0, limit = 0;
  uint64_t memo_direct = 0, memo_union = 0, work = 0;
  double seconds = 0, direct_seconds = 0, upper_seconds = 0;
};
struct PrepareRecord {
  uint64_t calls = 0, edits = 0, faces = 0;
  double seconds = 0;
};
struct Data {
  int rank = 0;
  std::array<PrepareRecord, size_t(SU2NativeTiming3D::Phase::Count)> prepare{};
  std::array<std::array<Queries, SiteCount>, 3> queries{};
  uint64_t stars = 0, candidates = 0, qj_calls = 0, shared_qj_calls = 0;
  double star_seconds = 0, candidate_seconds = 0, shared_qj_seconds = 0;
  std::array<uint64_t, WorkCount> work_calls{};
  std::array<double, WorkCount> work_seconds{}, qj_seconds{};
};
inline thread_local Data* active = nullptr;
inline thread_local unsigned candidate_depth = 0, star_depth = 0;
inline thread_local Work current_work = Work::Other;
inline int Phase() {
  const auto* frame = SU2NativeTiming3D::current_frame;
  if (!active || !frame) return -1;
  if (frame->phase == SU2NativeTiming3D::Phase::Quality) return 0;
  if (frame->phase == SU2NativeTiming3D::Phase::FinalProof) return 1;
  if (frame->phase == SU2NativeTiming3D::Phase::Surface) return 2;
  return -1;
}
inline double Seconds(Clock::time_point start) { return std::chrono::duration<double>(Clock::now() - start).count(); }
inline void PrintQueries(const Data& data) {
  constexpr const char* names[] = {"a", "b", "c", "d", "e", "g"};
  constexpr const char* phases[] = {"quality", "final_proof", "surface"};
  for (size_t phase = 0; phase < 3; ++phase) {
    for (size_t site = 0; site < SiteCount; ++site) {
      const auto& q = data.queries[phase][site];
      std::cout << "D9_QUERY rank=" << data.rank << " phase=" << phases[phase] << " site=" << names[site]
                << " scopes=" << q.scopes << " direct_calls=" << q.direct << " upper_calls=" << q.upper
                << " classification_calls=" << q.classify << " magnitude_calls=" << q.magnitude
                << " limit_calls=" << q.limit << " memo_direct_hits=" << q.memo_direct
                << " memo_union_hits=" << q.memo_union << " upper_work=" << q.work << " seconds=" << q.seconds
                << " direct_seconds=" << q.direct_seconds << " upper_seconds=" << q.upper_seconds << '\n';
    }
  }
}
inline void PrintMoves(const Data& data) {
  std::cout << "D9_MOVE rank=" << data.rank << " stars=" << data.stars << " candidates=" << data.candidates
            << " star_seconds=" << data.star_seconds << " candidate_seconds=" << data.candidate_seconds
            << " shared_qj_calls=" << data.shared_qj_calls << " shared_qj_seconds=" << data.shared_qj_seconds
            << " qj_calls=" << data.qj_calls << '\n';
  constexpr const char* names[] = {"other", "export", "projection", "move", "guidance"};
  for (size_t work = 0; work < WorkCount; ++work) {
    std::cout << "D9_MOVE_PART rank=" << data.rank << " part=" << names[work] << " calls=" << data.work_calls[work]
              << " inclusive_seconds=" << data.work_seconds[work] << " qj_seconds=" << data.qj_seconds[work] << '\n';
  }
}
// Times only the local union preparation, using the existing enabled Adapt diagnostic root.
struct Prepare {
  PrepareRecord* row = nullptr;
  Clock::time_point started{};
  Prepare(size_t edits, size_t faces) {
    const auto* frame = SU2NativeTiming3D::current_frame;
    if (!active || !frame || frame->phase == SU2NativeTiming3D::Phase::Count) return;
    row = &active->prepare[size_t(frame->phase)];
    ++row->calls;
    row->edits += edits;
    row->faces += faces;
    started = Clock::now();
  }
  ~Prepare() {
    if (row) row->seconds += Seconds(started);
  }
  Prepare(const Prepare&) = delete;
  Prepare& operator=(const Prepare&) = delete;
};
inline void PrintPrepare(const Data& data) {
  for (size_t stage = 0; stage < data.prepare.size(); ++stage) {
    const auto& row = data.prepare[stage];
    std::cout << "NATIVE3D_WALL_PREPARE rank=" << data.rank
              << " stage=" << SU2NativeTiming3D::PhaseName(SU2NativeTiming3D::Phase(stage)) << " calls=" << row.calls
              << " seconds=" << row.seconds << " edits=" << row.edits << " faces=" << row.faces << '\n';
  }
}
struct Root {
  Data data;
  Data* previous;
  unsigned previous_candidates, previous_stars;
  Work previous_work;
  bool enabled;
  Root(bool on, int rank)
      : previous(active),
        previous_candidates(candidate_depth),
        previous_stars(star_depth),
        previous_work(current_work),
        enabled(on) {
    if (enabled) {
      data.rank = rank;
      active = &data;
      candidate_depth = star_depth = 0;
      current_work = Work::Other;
    }
  }
  ~Root() {
    if (!enabled) return;
    const auto precision = std::cout.precision();
    std::cout << std::setprecision(17);
    PrintQueries(data);
    PrintMoves(data);
    PrintPrepare(data);
    std::cout.precision(precision);
    active = previous;
    candidate_depth = previous_candidates;
    star_depth = previous_stars;
    current_work = previous_work;
  }
  Root(const Root&) = delete;
  Root& operator=(const Root&) = delete;
};
struct Star {
  Data* data = Phase() == 0 ? active : nullptr;
  Clock::time_point start{};
  Star() {
    if (data) {
      ++data->stars;
      ++star_depth;
      start = Clock::now();
    }
  }
  ~Star() {
    if (data) {
      data->star_seconds += Seconds(start);
      --star_depth;
    }
  }
};
struct Candidate {
  Data* data = Phase() == 0 ? active : nullptr;
  Clock::time_point start{};
  Work previous = current_work;
  Candidate() {
    if (data) {
      ++data->candidates;
      ++candidate_depth;
      current_work = Work::Other;
      start = Clock::now();
    }
  }
  ~Candidate() {
    if (data) {
      data->candidate_seconds += Seconds(start);
      --candidate_depth;
      current_work = previous;
    }
  }
};
struct WorkScope {
  Data* data;
  Work work, previous = current_work;
  Clock::time_point start{};
  explicit WorkScope(Work value)
      : data(Phase() == 0 && (candidate_depth || value == Work::Export) ? active : nullptr), work(value) {
    if (data) {
      ++data->work_calls[size_t(work)];
      current_work = work;
      start = Clock::now();
    }
  }
  ~WorkScope() {
    if (data) {
      data->work_seconds[size_t(work)] += Seconds(start);
      current_work = previous;
    }
  }
};
template <class F>
auto Measure(Work work, F&& call) -> decltype(call()) {
  WorkScope scope(work);
  return call();
}
struct Quality {
  Data* data = Phase() == 0 && (candidate_depth || star_depth) ? active : nullptr;
  bool candidate = candidate_depth != 0;
  Work work = current_work;
  Clock::time_point start{};
  Quality() {
    if (data) start = Clock::now();
  }
  ~Quality() {
    if (!data) return;
    const auto seconds = Seconds(start);
    if (candidate) {
      ++data->qj_calls;
      data->qj_seconds[size_t(work)] += seconds;
    } else {
      ++data->shared_qj_calls;
      data->shared_qj_seconds += seconds;
    }
  }
};
}  // namespace SU2NativeD9
