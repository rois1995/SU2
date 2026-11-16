// Standalone I.B0 observation checks; no mesh, MPI, budget or tolerance is simulated.
#include "CNativeWallDiagnostics3D.hpp"
#include "CNativeD9Profile3D.hpp"
#include <cassert>
#include <sstream>
#include <string>

using namespace SU2NativeWallDiagnostics3D;

// Supplies actual refusal text and completed guard outcomes to one worker.
static void Observe(Counters& c) {
  c.ObserveGeometry(false);
  c.ObserveGeometry(true);
  c.ObserveGeometry(false);
  c.ObserveGuard("SplitEdge", GuardOrigin::Operator, false, true, true, true,
                 "marker junction violates orientation containment");
  c.ObserveGeometry(false);
  c.ObserveGuard("InsertEdgeCavity", GuardOrigin::Operator, false, true, true, true, "incomplete star");
  c.ObserveGeometry(false);
  c.ObserveGuard("CollapseVertex", GuardOrigin::Operator, true, false, false, false, "");
  c.ObserveGeometry(false);
  c.ObserveGuard("SplitEdge", GuardOrigin::Operator, true, true, false, false, "");
  c.ObserveGeometry(false);
  c.ObserveGuard("SplitEdge", GuardOrigin::Operator, true, true, true, false, "");
  c.ObserveGeometry(true);
  c.ObserveGuard("SplitEdge", GuardOrigin::Operator, true, true, true, true, "");
  c.ObserveGeometry(false);
  c.ObserveGuard("SplitEdge", GuardOrigin::Guidance, false, true, true, true, "");
  for (const auto* reason : {"Native3D projected split inverts inherited child connectivity.",
                             "Native 3D incomplete insertion dependency at interface face.",
                             "Native 3D surface insertion leaves or reverses its finite chart fan.", "feature mismatch",
                             "unclassified operator refusal"}) {
    c.ObserveGeometry(false);
    c.ObserveGuard("SplitEdge", GuardOrigin::Operator, false, true, true, true, reason);
  }
  c.GuidanceTrial(true);
  c.GuidanceTrial(false);
  c.GuidanceReject(GuidanceCause::Incomplete, true);
  c.GuidanceReject(GuidanceCause::AfterAbove, false);
  assert(!c.MetricValidation(false));
  assert(c.MetricValidation(true));
  c.MetricTrial(false, false);
  c.MetricTrial(true, false);
  c.MetricTrial(true, true);
  c.Accepted();
  c.PlanningRefusal();

  Choice choice(c);
  choice.Begin(true);
  choice.Disposition(PreTrialDisposition::Oversized);
  choice.EnterOperator();
  choice.Disposition(PreTrialDisposition::Conflict);
  choice.Begin(true);
  choice.Disposition(PreTrialDisposition::Deferred);
  Choice inactive(c);
  inactive.Begin(false);
  inactive.Disposition(PreTrialDisposition::Conflict);
}

// Checks disjoint categories, scope residuals and unchanged Boolean return decisions.
static void Check(const std::string& rows) {
  assert(rows.find("class=SplitEdge reason=marker_junction count=1") != std::string::npos);
  assert(rows.find("class=SplitEdge reason=orientation count=1") != std::string::npos);
  assert(rows.find("class=SplitEdge reason=interface count=1") != std::string::npos);
  assert(rows.find("class=SplitEdge reason=surface_containment count=1") != std::string::npos);
  assert(rows.find("class=SplitEdge reason=feature count=1") != std::string::npos);
  assert(rows.find("class=SplitEdge reason=other count=1") != std::string::npos);
  assert(rows.find("class=InsertEdgeCavity reason=incomplete_star count=1") != std::string::npos);
  assert(rows.find("class=PreservePassingFirstHeights count=1") != std::string::npos);
  assert(rows.find("class=height_check count=1") != std::string::npos);
  assert(rows.find("class=boundary_replacement_ready count=1") != std::string::npos);
  assert(rows.find("origin=geometry cause=incomplete count=1") != std::string::npos);
  assert(rows.find("origin=size cause=after_above count=1") != std::string::npos);
  assert(rows.find("disposition=conflict count=0") != std::string::npos);
  assert(rows.find("disposition=deferred count=1") != std::string::npos);
  assert(rows.find("geometry_trials=14 expected_geometry_trials=14 geometry_rejected=12") != std::string::npos);
  assert(rows.find("class=unwrapped_geometry_trial count=1") != std::string::npos);
  assert(rows.find(
             "guidance_trials=3 expected_guidance_trials=3 geometric_change_guidance_trials=2 priority_guidance=1") !=
         std::string::npos);
  assert(rows.find("metric_validation_trials=2 metric_validation_rejected=1 metric_rejected=2 metric_vetoed=1") !=
         std::string::npos);
  assert(
      rows.find(
          "selected=2 operator_entered=1 pretrial=1 choice_partition_sum=2 expected_proposed=2 expected_attempted=1") !=
      std::string::npos);
  assert(rows.find("refusal_partition_sum=12 counter_sums_match=1") != std::string::npos);
  assert(!Counters(false).MetricValidation(false));
}

// Verifies the added phase observes surface queries without enabling quality-only move work.
static void CheckSurfacePhase() {
  SU2NativeD9::Data data;
  SU2NativeTiming3D::Frame frame;
  auto* prior_data = SU2NativeD9::active;
  auto* prior_frame = SU2NativeTiming3D::current_frame;
  SU2NativeD9::active = &data;
  SU2NativeTiming3D::current_frame = &frame;
  frame.phase = SU2NativeTiming3D::Phase::Surface;
  assert(SU2NativeD9::Phase() == 2);
  {
    SU2NativeD9::Candidate candidate;
    SU2NativeD9::Star star;
  }
  assert(data.candidates == 0 && data.stars == 0);
  frame.phase = SU2NativeTiming3D::Phase::Quality;
  assert(SU2NativeD9::Phase() == 0);
  frame.phase = SU2NativeTiming3D::Phase::FinalProof;
  assert(SU2NativeD9::Phase() == 1);
  SU2NativeD9::active = prior_data;
  SU2NativeTiming3D::current_frame = prior_frame;
  data.queries[2][0].scopes = 7;
  std::ostringstream output;
  auto* previous = std::cout.rdbuf(output.rdbuf());
  SU2NativeD9::PrintQueries(data);
  std::cout.rdbuf(previous);
  assert(output.str().find("phase=surface site=a scopes=7") != std::string::npos);
  assert(output.str().find("phase=quality site=a scopes=0") != std::string::npos);
  assert(output.str().find("phase=final_proof site=a scopes=0") != std::string::npos);
}

// Captures one rank report; every assertion is about observation semantics only.
int main() {
  CheckSurfacePhase();
  Counters c(true);
  Observe(c);
  const Expected expected{14, 12, 3, 2, 1, 3, 2, 1};
  std::ostringstream output;
  auto* previous = std::cout.rdbuf(output.rdbuf());
  c.Print(0, expected);
  std::cout.rdbuf(previous);
  Check(output.str());
  return 0;
}
