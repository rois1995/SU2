/*!
 * \file CNativeTimerTree3D.hpp
 * \brief Fixed-storage, thread-local exclusive timers for native adaptation phases.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace SU2NativeTiming3D {

/*! Finite adaptation phases used as the first dimension of the timer ledger. */
enum class Phase : std::size_t {
  Preparation,
  Metric,
  Construction,
  Surface,
  Quality,
  FinalProof,
  Transfer,
  Output,
  Count
};

/*! Named operations recorded within each phase; Unattributed records phase-only scopes. */
enum class Operation : std::size_t {
  Unattributed,
  Import,
  Sensors,
  Hessian,
  TargetComposition,
  MobilityPolicy,
  Census,
  GeometryBound,
  GeometryPreparation,
  GeometryProjection,
  QualityOptimization,
  OldCavityMeasurement,
  Candidate,
  Scheduling,
  Topology,
  Communication,
  MetricNormalization,
  Certification,
  Output,
  Count
};

/*! One operation's fixed-storage timing totals; inclusive time is intentionally nonadditive. */
struct Record {
  double inclusive_call_seconds = 0.;
  double exclusive_seconds = 0.;
  std::uint64_t calls = 0;
};

/*! Snapshot of completed rows; coverage excludes root and phase-only unattributed time. */
struct SnapshotData {
  std::array<std::array<Record, static_cast<std::size_t>(Operation::Count)>, static_cast<std::size_t>(Phase::Count)>
      records{};
  double root_seconds = 0.;
  double root_exclusive_seconds = 0.;
  double sum_exclusive_seconds = 0.;
  double unattributed_seconds = 0.;
  double coverage = 0.;
};

class Session;

/*! Active stack frame; frames live inside the root session or lexical scope objects. */
struct Frame {
  Session* session = nullptr;
  Frame* parent = nullptr;
  Phase phase = Phase::Preparation;
  Operation operation = Operation::Count;
  std::chrono::steady_clock::time_point started{};
  double child_seconds = 0.;
};

/*! Thread-local active frame; a null pointer means timers are off on this thread. */
inline thread_local Frame* current_frame = nullptr;

/*! Return a stable textual name for a phase ledger row. */
inline const char* PhaseName(Phase phase) {
  switch (phase) {
    case Phase::Preparation:
      return "preparation";
    case Phase::Metric:
      return "metric";
    case Phase::Construction:
      return "construction";
    case Phase::Surface:
      return "surface";
    case Phase::Quality:
      return "quality";
    case Phase::FinalProof:
      return "final_proof";
    case Phase::Transfer:
      return "transfer";
    case Phase::Output:
      return "output";
    case Phase::Count:
      break;
  }
  return "invalid";
}

/*! Return a stable textual name for an operation ledger row. */
inline const char* OperationName(Operation operation) {
  switch (operation) {
    case Operation::Unattributed:
      return "unattributed";
    case Operation::Sensors:
      return "sensors";
    case Operation::Hessian:
      return "hessian";
    case Operation::Import:
      return "import";
    case Operation::TargetComposition:
      return "target_composition";
    case Operation::MobilityPolicy:
      return "mobility_policy";
    case Operation::Census:
      return "census";
    case Operation::GeometryBound:
      return "geometry_bound";
    case Operation::GeometryPreparation:
      return "geometry_preparation";
    case Operation::GeometryProjection:
      return "geometry_projection";
    case Operation::QualityOptimization:
      return "quality_optimization";
    case Operation::OldCavityMeasurement:
      return "old_cavity_measurement";
    case Operation::Scheduling:
      return "scheduling";
    case Operation::Candidate:
      return "candidate";
    case Operation::Topology:
      return "topology";
    case Operation::Communication:
      return "communication";
    case Operation::MetricNormalization:
      return "metric_normalization";
    case Operation::Certification:
      return "certification";
    case Operation::Output:
      return "output";
    case Operation::Count:
      break;
  }
  return "invalid";
}

/*! Root ledger and local stdout report; nested sessions borrow the active root. */
class Session {
 public:
  using Clock = std::chrono::steady_clock;
  static constexpr std::size_t PhaseCount = static_cast<std::size_t>(Phase::Count);
  static constexpr std::size_t OperationCount = static_cast<std::size_t>(Operation::Count);

  /*! Create the only root on a thread when enabled; disabled sessions do no timing work. */
  Session(bool enabled, const char* root_name, int rank, int ranks, Phase initial_phase)
      : root_name_(root_name), rank_(rank), ranks_(ranks) {
    if (!enabled || current_frame != nullptr) return;
    if (initial_phase == Phase::Count) throw std::invalid_argument("Invalid initial timer phase.");
    owns_ = true;
    root_frame_.session = this;
    root_frame_.phase = initial_phase;
    root_frame_.started = Clock::now();
    current_frame = &root_frame_;
  }

  /*! Close and locally report an owning session; borrowed and disabled sessions are silent. */
  ~Session() {
    if (!owns_) return;
    if (!finished_ && current_frame == &root_frame_) Finalize();
    if (finished_) Report("final");
  }

  /*! Keep the root frame address stable for all active lexical scopes. */
  Session(const Session&) = delete;
  /*! Keep the root frame address stable for all active lexical scopes. */
  Session& operator=(const Session&) = delete;
  /*! Keep the root frame address stable for all active lexical scopes. */
  Session(Session&&) = delete;
  /*! Keep the root frame address stable for all active lexical scopes. */
  Session& operator=(Session&&) = delete;

  /*! Return whether this object owns the active root ledger. */
  bool Owns() const { return owns_; }

  /*! Return fixed bytes occupied by one root session, excluding caller-owned lexical frames. */
  static constexpr std::size_t ReservedBytes() { return sizeof(Session); }

  /*! Finalize only after every lexical child scope has restored the root frame. */
  void Finish() {
    if (!owns_ || finished_) return;
    if (current_frame != &root_frame_) throw std::logic_error("Timer session finished with open scopes.");
    Finalize();
  }

  /*! Emit a nonfinal cumulative snapshot at a resumable boundary without closing live scopes. */
  static void Checkpoint(const char* boundary) {
    if (current_frame != nullptr) current_frame->session->Report(boundary);
  }

  /*! Copy the fixed ledger and root totals without allocating memory. */
  SnapshotData Snapshot() const {
    SnapshotData result;
    if (!owns_) return result;
    result.records = records_;
    const auto now = finished_ ? Clock::time_point{} : Clock::now();
    const auto active_child_seconds = finished_ ? 0. : AddOpenFrames(result, now);
    result.root_seconds = finished_ ? root_seconds_ : Seconds(root_frame_.started, now);
    const auto open_root_exclusive = result.root_seconds - root_frame_.child_seconds - active_child_seconds;
    result.root_exclusive_seconds =
        finished_ ? root_exclusive_seconds_ : (open_root_exclusive > 0. ? open_root_exclusive : 0.);
    result.sum_exclusive_seconds = result.root_exclusive_seconds;
    result.unattributed_seconds = result.root_exclusive_seconds;
    for (std::size_t phase = 0; phase < PhaseCount; ++phase) {
      for (std::size_t operation = 0; operation < OperationCount; ++operation) {
        const auto& record = result.records[phase][operation];
        result.sum_exclusive_seconds += record.exclusive_seconds;
        if (operation == static_cast<std::size_t>(Operation::Unattributed))
          result.unattributed_seconds += record.exclusive_seconds;
      }
    }
    if (result.root_seconds > 0.)
      result.coverage = (result.root_seconds - result.unattributed_seconds) / result.root_seconds;
    return result;
  }

 private:
  friend class Scope;
  using Ledger = std::array<std::array<Record, OperationCount>, PhaseCount>;

  /*! Include active lexical frames in a snapshot; no ledger mutation or allocation is needed. */
  double AddOpenFrames(SnapshotData& snapshot, Clock::time_point now) const {
    double active_child_seconds = 0.;
    for (const auto* frame = current_frame; frame != &root_frame_; frame = frame->parent) {
      const auto elapsed = Seconds(frame->started, now);
      auto& record =
          snapshot.records[static_cast<std::size_t>(frame->phase)][static_cast<std::size_t>(frame->operation)];
      record.inclusive_call_seconds += elapsed;
      record.exclusive_seconds += std::max(0., elapsed - frame->child_seconds - active_child_seconds);
      ++record.calls;
      active_child_seconds = elapsed;
    }
    return active_child_seconds;
  }

  /*! Convert a steady-clock interval to seconds. */
  static double Seconds(Clock::time_point start, Clock::time_point end) {
    return std::chrono::duration<double>(end - start).count();
  }

  /*! Record a completed frame in its phase/operation cell. */
  void Add(Phase phase, Operation operation, double elapsed, double exclusive) {
    auto& record = records_[static_cast<std::size_t>(phase)][static_cast<std::size_t>(operation)];
    record.inclusive_call_seconds += elapsed;
    record.exclusive_seconds += exclusive;
    ++record.calls;
  }

  /*! Finalize root totals and remove this session from the thread-local context. */
  void Finalize() {
    root_seconds_ = Seconds(root_frame_.started, Clock::now());
    const auto exclusive = root_seconds_ - root_frame_.child_seconds;
    root_exclusive_seconds_ = exclusive > 0. ? exclusive : 0.;
    finished_ = true;
    current_frame = nullptr;
  }

  /*! Emit local rows; inclusive call totals are nonadditive across nested calls. */
  void Report(const char* boundary) const {
    const auto snapshot = Snapshot();
    const auto precision = std::cout.precision();
    std::cout << std::setprecision(17);
    for (std::size_t phase = 0; phase < PhaseCount; ++phase) {
      for (std::size_t operation = 0; operation < OperationCount; ++operation) {
        const auto& record = snapshot.records[phase][operation];
        if (record.calls == 0) continue;
        std::cout << "NATIVE3D_TIMER root=" << root_name_ << " boundary=" << boundary << " rank=" << rank_
                  << " phase=" << PhaseName(static_cast<Phase>(phase))
                  << " operation=" << OperationName(static_cast<Operation>(operation)) << " calls=" << record.calls
                  << " inclusive_call_seconds=" << record.inclusive_call_seconds
                  << " exclusive_seconds=" << record.exclusive_seconds << '\n';
      }
    }
    std::cout << "NATIVE3D_TIMER_SUMMARY root=" << root_name_ << " boundary=" << boundary << " rank=" << rank_
              << " ranks=" << ranks_ << " root_seconds=" << snapshot.root_seconds
              << " sumexclusive=" << snapshot.sum_exclusive_seconds << " unattributed=" << snapshot.unattributed_seconds
              << " coverage=" << snapshot.coverage << " fixedstorage=" << ReservedBytes() << '\n';
    std::cout.precision(precision);
  }

  const char* root_name_ = "native";
  int rank_ = 0;
  int ranks_ = 1;
  bool owns_ = false;
  bool finished_ = false;
  double root_seconds_ = 0.;
  double root_exclusive_seconds_ = 0.;
  Frame root_frame_{};
  Ledger records_{};
};

/*! Nested lexical operation timer; child wall time is removed from parent exclusives. */
class Scope {
 public:
  /*! Start an operation in the current phase, or remain inert when no root is active. */
  explicit Scope(Operation operation) : Scope(operation, Phase::Count, false) {}

  /*! Restore the prior frame and add inclusive/exclusive totals, including during unwinding. */
  ~Scope() {
    if (!active_) return;
    const auto elapsed = Session::Seconds(frame_.started, Session::Clock::now());
    const auto exclusive = elapsed > frame_.child_seconds ? elapsed - frame_.child_seconds : 0.;
    frame_.session->Add(frame_.phase, frame_.operation, elapsed, exclusive);
    frame_.parent->child_seconds += elapsed;
    current_frame = frame_.parent;
  }

  /*! Preserve strict LIFO frame restoration by forbidding copies and moves. */
  Scope(const Scope&) = delete;
  /*! Preserve strict LIFO frame restoration by forbidding copies and moves. */
  Scope& operator=(const Scope&) = delete;
  /*! Preserve strict LIFO frame restoration by forbidding copies and moves. */
  Scope(Scope&&) = delete;
  /*! Preserve strict LIFO frame restoration by forbidding copies and moves. */
  Scope& operator=(Scope&&) = delete;

 private:
  friend class PhaseScope;

  /*! Start an operation under an explicit phase for PhaseScope's phase-only node. */
  Scope(Operation operation, Phase phase, bool override_phase) {
    if (operation >= Operation::Count) throw std::invalid_argument("Invalid timer operation.");
    if (override_phase && phase >= Phase::Count) throw std::invalid_argument("Invalid timer phase.");
    if (current_frame == nullptr) return;
    frame_.session = current_frame->session;
    frame_.parent = current_frame;
    frame_.phase = override_phase ? phase : current_frame->phase;
    frame_.operation = operation;
    frame_.started = Session::Clock::now();
    current_frame = &frame_;
    active_ = true;
  }

  Frame frame_{};
  bool active_ = false;
};

/*! Phase-only lexical node; its operation scopes inherit the selected phase. */
class PhaseScope {
 public:
  /*! Record an unattributed phase frame and restore the previous phase on scope exit. */
  explicit PhaseScope(Phase phase) : scope_(Operation::Unattributed, phase, true) {}

  /*! Preserve the contained scope frame's lexical lifetime and restore order. */
  PhaseScope(const PhaseScope&) = delete;
  /*! Preserve the contained scope frame's lexical lifetime and restore order. */
  PhaseScope& operator=(const PhaseScope&) = delete;
  /*! Preserve the contained scope frame's lexical lifetime and restore order. */
  PhaseScope(PhaseScope&&) = delete;
  /*! Preserve the contained scope frame's lexical lifetime and restore order. */
  PhaseScope& operator=(PhaseScope&&) = delete;

 private:
  Scope scope_;
};

}  // namespace SU2NativeTiming3D
