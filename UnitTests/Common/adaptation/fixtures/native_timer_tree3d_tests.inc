/*!
 * \file native_timer_tree3d_tests.inc
 * \brief Focused correctness checks for fixed-storage native timer tree scopes.
 * The including test translation unit must provide Catch2's TEST_CASE and CHECK macros.
 */
#include "../../../../Common/include/adaptation/CNativeTimerTree3D.hpp"
#include <sstream>
#include <string>

/*! Verify nested exclusive totals telescope to the root and preserve phase records. */
TEST_CASE("Native timer tree telescopes nested exclusive scopes", "[NativeTimerTree3D]") {
  using namespace SU2NativeTiming3D;
  Session session(true, "tree_test", 0, 1, Phase::Preparation);
  {
    Scope outer(Operation::Census);
    { Scope inner(Operation::GeometryBound); }
  }
  {
    PhaseScope phase(Phase::Metric);
    Scope metric(Operation::MetricNormalization);
  }
  session.Finish();
  const auto snapshot = session.Snapshot();
  CHECK(snapshot.records[static_cast<std::size_t>(Phase::Preparation)][static_cast<std::size_t>(Operation::Census)]
            .calls == 1);
  CHECK(snapshot.records[static_cast<std::size_t>(Phase::Metric)][static_cast<std::size_t>(Operation::Unattributed)]
            .calls == 1);
  CHECK(snapshot.sum_exclusive_seconds == Catch::Detail::Approx(snapshot.root_seconds).epsilon(1e-8));
  CHECK(snapshot.coverage >= 0.);
  CHECK(snapshot.coverage <= 1.);
}

/*! Verify repeated operation keys retain calls without adding nested inclusive totals. */
TEST_CASE("Native timer tree counts nested scopes of the same operation", "[NativeTimerTree3D]") {
  using namespace SU2NativeTiming3D;
  Session session(true, "same_kind_test", 0, 1, Phase::Construction);
  {
    Scope outer(Operation::Candidate);
    { Scope inner(Operation::Candidate); }
  }
  session.Finish();
  const auto snapshot = session.Snapshot();
  const auto& record =
      snapshot.records[static_cast<std::size_t>(Phase::Construction)][static_cast<std::size_t>(Operation::Candidate)];
  CHECK(record.calls == 2);
  CHECK(record.inclusive_call_seconds >= record.exclusive_seconds);
}

/*! Verify phase context returns to its enclosing phase after PhaseScope exits. */
TEST_CASE("Native timer tree restores phase after PhaseScope", "[NativeTimerTree3D]") {
  using namespace SU2NativeTiming3D;
  Session session(true, "phase_restore_test", 0, 1, Phase::Preparation);
  {
    PhaseScope metric(Phase::Metric);
    Scope composition(Operation::TargetComposition);
  }
  { Scope census(Operation::Census); }
  session.Finish();
  const auto snapshot = session.Snapshot();
  CHECK(
      snapshot.records[static_cast<std::size_t>(Phase::Metric)][static_cast<std::size_t>(Operation::TargetComposition)]
          .calls == 1);
  CHECK(snapshot.records[static_cast<std::size_t>(Phase::Preparation)][static_cast<std::size_t>(Operation::Census)]
            .calls == 1);
}

/*! Verify exception unwinding closes nested frames and restores the enclosing phase. */
TEST_CASE("Native timer tree restores frames while unwinding exceptions", "[NativeTimerTree3D]") {
  using namespace SU2NativeTiming3D;
  Session session(true, "exception_test", 0, 1, Phase::Construction);
  try {
    PhaseScope surface(Phase::Surface);
    Scope candidate(Operation::Candidate);
    throw std::runtime_error("scope unwind");
  } catch (const std::runtime_error&) {
  }
  { Scope proof(Operation::Certification); }
  session.Finish();
  const auto snapshot = session.Snapshot();
  CHECK(snapshot.records[static_cast<std::size_t>(Phase::Surface)][static_cast<std::size_t>(Operation::Candidate)]
            .calls == 1);
  CHECK(snapshot
            .records[static_cast<std::size_t>(Phase::Construction)][static_cast<std::size_t>(Operation::Certification)]
            .calls == 1);
}

/*! Verify disabled roots and scopes do not time, record, or emit rows. */
TEST_CASE("Disabled native timer tree is silent and records nothing", "[NativeTimerTree3D]") {
  using namespace SU2NativeTiming3D;
  std::ostringstream captured;
  auto* original = std::cout.rdbuf(captured.rdbuf());
  SnapshotData snapshot;
  bool owns = true;
  {
    Session session(false, "disabled_test", 0, 1, Phase::Preparation);
    owns = session.Owns();
    { Scope census(Operation::Census); }
    { PhaseScope metric(Phase::Metric); }
    snapshot = session.Snapshot();
  }
  std::cout.rdbuf(original);
  CHECK_FALSE(owns);
  CHECK(snapshot.root_seconds == 0.);
  CHECK(snapshot.sum_exclusive_seconds == 0.);
  CHECK(captured.str().empty());
}

/*! Verify nested sessions record under the outer owner and never emit a second root. */
TEST_CASE("Nested native timer sessions borrow the outer root", "[NativeTimerTree3D]") {
  using namespace SU2NativeTiming3D;
  std::ostringstream captured;
  auto* original = std::cout.rdbuf(captured.rdbuf());
  SnapshotData snapshot;
  bool nested_owns = true;
  {
    Session root(true, "borrow_test", 0, 1, Phase::Preparation);
    {
      Session nested(true, "nested_test", 0, 1, Phase::Quality);
      nested_owns = nested.Owns();
      Scope census(Operation::Census);
    }
    root.Finish();
    snapshot = root.Snapshot();
  }
  std::cout.rdbuf(original);
  CHECK_FALSE(nested_owns);
  CHECK(snapshot.records[static_cast<std::size_t>(Phase::Preparation)][static_cast<std::size_t>(Operation::Census)]
            .calls == 1);
  CHECK(captured.str().find("root=borrow_test") != std::string::npos);
  CHECK(captured.str().find("root=nested_test") == std::string::npos);
}

/*! Live snapshots must telescope before nested frames have closed at an intentional stop boundary. */
TEST_CASE("Native timer snapshots include open frames", "[NativeTimerTree3D]") {
  using namespace SU2NativeTiming3D;
  Session session(true, "live_test", 0, 1, Phase::Construction);
  Scope census(Operation::Census);
  Scope geometry(Operation::GeometryBound);
  const auto snapshot = session.Snapshot();
  CHECK(snapshot.sum_exclusive_seconds == Catch::Detail::Approx(snapshot.root_seconds).epsilon(1e-8));
  CHECK(snapshot.records[static_cast<std::size_t>(Phase::Construction)][static_cast<std::size_t>(Operation::Census)]
            .calls == 1);
}

/*! Checkpoint rows include active frames; only the terminal report uses the final boundary label. */
TEST_CASE("Native timer checkpoints emit active operation rows", "[NativeTimerTree3D]") {
  using namespace SU2NativeTiming3D;
  std::ostringstream captured;
  auto* original = std::cout.rdbuf(captured.rdbuf());
  {
    Session session(true, "checkpoint_test", 0, 1, Phase::Construction);
    Scope census(Operation::Census);
    Session::Checkpoint("collapse");
  }
  std::cout.rdbuf(original);
  const auto report = captured.str();
  CHECK(report.find("boundary=collapse rank=0 phase=construction operation=census calls=1") != std::string::npos);
  CHECK(report.find("boundary=final rank=0") != std::string::npos);
}
