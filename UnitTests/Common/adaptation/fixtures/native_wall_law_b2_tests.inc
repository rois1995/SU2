#include "../../../../Common/include/adaptation/CNativeWallGeometryLaw3D.hpp"
#include "../../../../Common/include/adaptation/CNativeWallDiagnostics3D.hpp"
#include "../../../../Common/include/adaptation/CNativeTimerTree3D.hpp"
#include <sstream>

TEST_CASE("Wall face law weights normalize all three edges or fall back together", "[NativeWallLawB2]") {
  using namespace SU2Native3D;
  const std::array<WallEdgeEstimate, 3> estimates{{{true, 1}, {true, 2}, {true, 4}}};
  const auto weights = WallFaceEdgeWeights(estimates);
  REQUIRE(weights.weighted);
  CHECK(weights.factors[0] == 1.L / 16);
  CHECK(weights.factors[1] == 1.L / 4);
  CHECK(weights.factors[2] == 1);
  // A longer low-curvature edge must lose to the shorter high-curvature edge.
  CHECK(4.L * weights.factors[0] < 1.L * weights.factors[2]);
  auto unsupported = estimates;
  unsupported[1].supported = false;
  const auto fallback = WallFaceEdgeWeights(unsupported);
  CHECK_FALSE(fallback.weighted);
  for (auto factor : fallback.factors) CHECK(factor == 1);
  const auto zero = WallFaceEdgeWeights({{{true, 0}, {true, 0}, {true, 0}}});
  CHECK_FALSE(zero.weighted);
  for (auto factor : zero.factors) CHECK(factor == 1);
  const auto partly_zero = WallFaceEdgeWeights({{{true, 0}, {true, 2}, {true, 0}}});
  CHECK(partly_zero.weighted);
  CHECK(partly_zero.factors[0] == 0);
  CHECK(partly_zero.factors[1] == 1);
  CHECK(partly_zero.factors[2] == 0);
}

TEST_CASE("Wall patch source census excludes false probes and derived repairs", "[NativeWallLawB2]") {
  using namespace SU2NativeWallDiagnostics3D;
  Counters counters(true), disabled(false);
  for (auto* target : {&counters, &disabled}) {
    target->ObserveLawFace(true);
    target->ObserveLawFace(false);
    target->ObserveSplit(true);
    target->ObserveSplit(false);
    target->ObserveSource(PatchSource::Primary, true, true, false);
    target->ObserveSource(PatchSource::Primary, false, true, false);
    target->ObserveSource(PatchSource::RefitWallSplit, true, false, true);
    target->ObserveSource(PatchSource::ReconstructWallSplit, true, false, false);
    target->ObserveSource(PatchSource::ReconstructSplitPatch, true, false, true);
    target->ObserveSource(PatchSource::Unreported, true, true, true);
  }
  std::ostringstream output;
  auto* previous = std::cout.rdbuf(output.rdbuf());
  counters.Print(0, Expected{});
  disabled.Print(99, Expected{});
  std::cout.rdbuf(previous);
  const auto rows = output.str();
  CHECK(rows.find("source=primary validated=1 guidance_rejected=1 approved=0") != std::string::npos);
  CHECK(rows.find("source=RefitWallSplit validated=1 guidance_rejected=0 approved=1") != std::string::npos);
  CHECK(rows.find("source=ReconstructWallSplit validated=1 guidance_rejected=0 approved=0") != std::string::npos);
  CHECK(rows.find("source=ReconstructSplitPatch validated=1 guidance_rejected=0 approved=1") != std::string::npos);
  CHECK(rows.find("law_weighted_faces=1 law_fallback_faces=1 accepted_geometry_splits=1 accepted_size_splits=1") !=
        std::string::npos);
  CHECK(rows.find("rank=99") == std::string::npos);
  CHECK(sizeof(SU2NativeTiming3D::Session) == 4120);
}

TEST_CASE("Split point lift requires a split strictly above tolerance", "[NativeWallGeometryLaw3D]") {
  using namespace SU2Native3D;
  const Point chord{1, 2, 3};
  CHECK_FALSE(SplitPointLiftRequiresSplit(chord, {1, 2, 3.5}, 1));
  CHECK_FALSE(SplitPointLiftRequiresSplit(chord, {1, 2, 4}, 1));
  CHECK(SplitPointLiftRequiresSplit(chord, {1, 2, 4.5}, 1));
  CHECK(SplitPointLiftRequiresSplit(chord, {2, 3, 3}, 1));
  CHECK_FALSE(SplitPointLiftRequiresSplit(chord, chord, 1));
}
