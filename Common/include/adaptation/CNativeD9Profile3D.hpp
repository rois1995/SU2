/* Diagnostic-only D9 observations. Owns event-local counters and the observed union faces, never mesh state,
 * geometry queries, admission, collectives or acceptance decisions. */
#pragma once
#include "CNativeTimerTree3D.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <unordered_map>

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

// M27 sources inside the existing query sites, or the site itself outside guidance calls.
enum class Part {
  None,
  GuidanceOther,
  GuidanceOld,
  GuidanceFresh,
  SmoothOther,
  SmoothOld,
  SmoothCandidate,
  Shell,
  Coupled,
  Census,
  Finish,
  Count
};
constexpr size_t PartCount = size_t(Part::Count);
constexpr size_t StageCount = 4;  // Other, quality, final proof, surface.

// Tracks answer kinds already computed for a purpose-independent face.
enum class Answer : uint8_t { Classified = 1, Magnitude = 2, CutOff = 4 };

// Miss arrays distinguish the first computation, the same answer kind and a different kind.
struct UnionRow {
  uint64_t queries = 0, hits = 0, bypass = 0;
  std::array<uint64_t, 3> misses{}, work{};
  std::array<double, 3> seconds{};
  uint64_t faces_reached_census = 0, work_reached_census = 0;
};

// Retains the first computation's attribution until the event ends or Census sees the face.
struct SeenFace {
  uint8_t stage = 0, part = 0, answers = 0;
  bool reached_census = false;
  uint64_t first_work = 0;
};

// Counts the exact checks reached by each wall-vertex search.
struct SmoothChains {
  uint64_t calls = 0, checked = 0, passed = 0, moved = 0;
  std::array<uint64_t, 5> checked_bins{};
  std::array<uint64_t, 4> passed_bins{};
};

struct Data {
  int rank = 0;
  std::array<PrepareRecord, size_t(SU2NativeTiming3D::Phase::Count)> prepare{};
  std::array<std::array<Queries, SiteCount>, 3> queries{};
  uint64_t stars = 0, candidates = 0, qj_calls = 0, shared_qj_calls = 0;
  double star_seconds = 0, candidate_seconds = 0, shared_qj_seconds = 0;
  std::array<uint64_t, WorkCount> work_calls{};
  std::array<double, WorkCount> work_seconds{}, qj_seconds{};
  std::array<std::array<UnionRow, PartCount>, StageCount> unions{};
  std::unordered_map<uint64_t, SeenFace> seen_faces;
  uint64_t faces_from_earlier_event = 0;
  bool union_observer_failed = false;
  SmoothChains chains;
};
inline thread_local Data* active = nullptr;
inline thread_local unsigned candidate_depth = 0, star_depth = 0;
inline thread_local Work current_work = Work::Other;
inline thread_local Part current_part = Part::None;
inline int Phase() {
  const auto* frame = SU2NativeTiming3D::current_frame;
  if (!active || !frame) return -1;
  if (frame->phase == SU2NativeTiming3D::Phase::Quality) return 0;
  if (frame->phase == SU2NativeTiming3D::Phase::FinalProof) return 1;
  if (frame->phase == SU2NativeTiming3D::Phase::Surface) return 2;
  return -1;
}
inline double Seconds(Clock::time_point start) { return std::chrono::duration<double>(Clock::now() - start).count(); }

// Names the innermost query source while the existing diagnostic root is active.
struct PartScope {
  Part previous = current_part;
  explicit PartScope(Part value) noexcept {
    if (active) current_part = value;
  }
  ~PartScope() { current_part = previous; }
  PartScope(const PartScope&) = delete;
  PartScope& operator=(const PartScope&) = delete;
};

// Supplies the default part for queries outside a narrower guidance scope.
inline Part SitePart(Site site) noexcept {
  switch (site) {
    case Site::Guidance:
      return Part::GuidanceOther;
    case Site::SmoothBoundary:
      return Part::SmoothOther;
    case Site::ShellBoundary:
      return Part::Shell;
    case Site::CoupledBoundary:
      return Part::Coupled;
    case Site::Census:
      return Part::Census;
    case Site::Finish:
      return Part::Finish;
    case Site::Count:
      return Part::None;
  }
  return Part::None;
}

// Observation stops after an allocation failure and remains off without timings.
inline bool ObservingUnion() noexcept { return active && !active->union_observer_failed; }

// Credits the first computation once when an already seen face reaches Census.
inline void MarkUnionCensus(SeenFace& face) noexcept {
  if (current_part != Part::Census || face.reached_census) return;
  face.reached_census = true;
  if (face.stage == StageCount) return;
  auto& first = active->unions[face.stage][face.part];
  ++first.faces_reached_census;
  first.work_reached_census += face.first_work;
}

// Counts calls that skip the memo without classifying them as computed misses.
inline void ObserveUnionBypass() noexcept {
  if (!ObservingUnion()) return;
  ++active->unions[size_t(Phase() + 1)][size_t(current_part)].bypass;
}

// Records a memo hit and marks faces first observed from an existing memo answer.
inline void ObserveUnionHit(uint64_t word, Answer wanted) noexcept {
  if (!ObservingUnion()) return;
  try {
    auto& row = active->unions[size_t(Phase() + 1)][size_t(current_part)];
    ++row.queries;
    ++row.hits;
    const auto [face, inserted] = active->seen_faces.try_emplace(word, SeenFace{StageCount, 0, uint8_t(wanted)});
    active->faces_from_earlier_event += inserted;
    MarkUnionCensus(face->second);
  } catch (...) {
    active->union_observer_failed = true;
  }
}

// Classifies the existing query work by prior computation and the requested answer kind.
inline void ObserveUnionMiss(uint64_t word, Answer wanted, Answer given, uint64_t work, double seconds) noexcept {
  if (!ObservingUnion()) return;
  try {
    const auto stage = uint8_t(Phase() + 1);
    const auto part = uint8_t(current_part);
    auto& row = active->unions[stage][part];
    ++row.queries;
    const auto [face, inserted] =
        active->seen_faces.try_emplace(word, SeenFace{stage, part, uint8_t(given), current_part == Part::Census, work});
    auto& seen = face->second;
    const size_t miss_class = inserted ? 0 : (seen.answers & uint8_t(wanted)) != 0 ? 1 : 2;
    ++row.misses[miss_class];
    row.work[miss_class] += work;
    row.seconds[miss_class] += seconds;
    if (!inserted) {
      seen.answers |= uint8_t(given);
      MarkUnionCensus(seen);
    }
  } catch (...) {
    active->union_observer_failed = true;
  }
}

// Commits one quality-stage search's diagnostic counts, including early returns.
struct SmoothChain {
  size_t checked = 0, passed = 0;
  bool moved = false;
  ~SmoothChain() {
    if (!ObservingUnion() || Phase() != 0 || !checked) return;
    auto& chains = active->chains;
    ++chains.calls;
    chains.checked += checked;
    chains.passed += passed;
    chains.moved += moved;
    ++chains.checked_bins[std::min(checked, size_t(5)) - 1];
    ++chains.passed_bins[std::min(passed, size_t(3))];
  }
};

// Prints one nonempty union-query row without altering the existing D9 log format.
inline void PrintUnionRow(int rank, const char* stage, const char* part, const UnionRow& row) {
  std::cout << "NATIVE3D_M27_QUERY rank=" << rank << " stage=" << stage << " part=" << part
            << " queries=" << row.queries << " hits=" << row.hits << " bypass=" << row.bypass
            << " miss_new=" << row.misses[0] << " miss_repeat=" << row.misses[1] << " miss_other_kind=" << row.misses[2]
            << " work_new=" << row.work[0] << " work_repeat=" << row.work[1] << " work_other_kind=" << row.work[2]
            << " seconds_new=" << row.seconds[0] << " seconds_repeat=" << row.seconds[1]
            << " seconds_other_kind=" << row.seconds[2] << " faces_reached_census=" << row.faces_reached_census
            << " work_reached_census=" << row.work_reached_census << '\n';
}

// Reports each event's union counts and estimated hash-table storage from the active D9 root.
inline void PrintUnionMeasurements(Data& data) noexcept {
  constexpr const char* stages[] = {"other", "quality", "final_proof", "surface"};
  constexpr const char* parts[] = {"none",        "a_other", "a_old", "a_fresh", "b_other", "b_old",
                                   "b_candidate", "c",       "d",     "e",       "g"};
  try {
    for (size_t stage = 0; stage < StageCount; ++stage) {
      for (size_t part = 0; part < PartCount; ++part) {
        const auto& row = data.unions[stage][part];
        if (row.queries || row.bypass) PrintUnionRow(data.rank, stages[stage], parts[part], row);
      }
    }
    const auto& chains = data.chains;
    std::cout << "NATIVE3D_M27_SMOOTH rank=" << data.rank << " calls=" << chains.calls << " checked=" << chains.checked
              << " passed=" << chains.passed << " moved=" << chains.moved << " checked_1=" << chains.checked_bins[0]
              << " checked_2=" << chains.checked_bins[1] << " checked_3=" << chains.checked_bins[2]
              << " checked_4=" << chains.checked_bins[3] << " checked_5plus=" << chains.checked_bins[4]
              << " passed_0=" << chains.passed_bins[0] << " passed_1=" << chains.passed_bins[1]
              << " passed_2=" << chains.passed_bins[2] << " passed_3plus=" << chains.passed_bins[3] << '\n';
    const auto& table = data.seen_faces;
    const auto bytes = sizeof(table) + table.size() * (sizeof(decltype(data.seen_faces)::value_type) + sizeof(void*)) +
                       table.bucket_count() * sizeof(void*);
    std::cout << "NATIVE3D_M27_TABLE rank=" << data.rank << " faces=" << table.size()
              << " from_earlier_event=" << data.faces_from_earlier_event
              << " observer_failed=" << data.union_observer_failed << " estimated_bytes=" << bytes << '\n';
  } catch (...) {
    data.union_observer_failed = true;
  }
}

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
    PrintUnionMeasurements(data);
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
