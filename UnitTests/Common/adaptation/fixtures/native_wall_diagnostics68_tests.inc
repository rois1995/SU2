#include "../../../../Common/include/adaptation/CNativeWallDiagnostics3D.hpp"
#include "../../../../Common/include/adaptation/CNativeTimerTree3D.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

TEST_CASE("Native wall diagnostic refusal classes stay disjoint", "[NativeWallDiagnostics68]") {
  using SU2NativeWallDiagnostics3D::Counters;
  using SU2NativeWallDiagnostics3D::Expected;
  using SU2NativeWallDiagnostics3D::GuardOrigin;
  Counters counters(true);
  for (const auto* operation : {"SplitEdge", "InsertEdgeCavity", "CollapseVertex", "RefitWallSplit",
                                "ReconstructWallSplit", "ReconstructSplitPatch"})
    counters.ObserveGuard(operation, GuardOrigin::Operator, false, true, true, true, "orientation refused");
  std::ostringstream output;
  auto* previous = std::cout.rdbuf(output.rdbuf());
  counters.Print(0, Expected{});
  std::cout.rdbuf(previous);
  const auto rows = output.str();
  CHECK(rows.find("class=SplitEdge reason=orientation count=1") != std::string::npos);
  CHECK(rows.find("class=InsertEdgeCavity reason=orientation count=1") != std::string::npos);
  CHECK(rows.find("class=other_geometry reason=orientation count=1") != std::string::npos);
  CHECK(rows.find("class=RefitWallSplit reason=orientation count=1") != std::string::npos);
  CHECK(rows.find("class=ReconstructWallSplit reason=orientation count=1") != std::string::npos);
  CHECK(rows.find("class=ReconstructSplitPatch reason=orientation count=1") != std::string::npos);
  unsigned partition = 0;
  std::istringstream lines(rows);
  std::string line;
  while (std::getline(lines, line)) {
    const bool other = line.find("class=other_geometry ") != std::string::npos;
    const bool refit = line.find("class=RefitWallSplit ") != std::string::npos;
    const bool wall = line.find("class=ReconstructWallSplit ") != std::string::npos;
    const bool patch = line.find("class=ReconstructSplitPatch ") != std::string::npos;
    if (other || refit || wall || patch) partition += std::stoul(line.substr(line.find("count=") + 6));
  }
  CHECK(partition == 4);
}

TEST_CASE("Native wall reason keys fold digits within the retained prefix", "[NativeWallDiagnostics68]") {
  using SU2NativeWallDiagnostics3D::Counters;
  using SU2NativeWallDiagnostics3D::Expected;
  using SU2NativeWallDiagnostics3D::GuardOrigin;
  Counters counters(true);
  counters.ObserveGuard("RefitWallSplit", GuardOrigin::Operator, false, true, true, true,
                        "facet 12345 cell 67890 geometry refusal");
  counters.ObserveGuard("RefitWallSplit", GuardOrigin::Operator, false, true, true, true,
                        "facet 9876 cell 54321 geometry refusal");
  const std::string leading_digits(95, '7');
  counters.ObserveGuard("RefitWallSplit", GuardOrigin::Operator, false, true, true, true,
                        leading_digits + "x-tail-one");
  counters.ObserveGuard("RefitWallSplit", GuardOrigin::Operator, false, true, true, true,
                        leading_digits + "x-tail-two");
  const std::string prefix(96, 'x');
  counters.ObserveGuard("RefitWallSplit", GuardOrigin::Operator, false, true, true, true, prefix + "tail-a");
  counters.ObserveGuard("RefitWallSplit", GuardOrigin::Operator, false, true, true, true, prefix + "tail-b");
  counters.ObserveGuard("RefitWallSplit", GuardOrigin::Guidance, false, true, true, true, "guidance-only text");
  counters.ObserveGuard("ReconstructWallSplit", GuardOrigin::Operator, true, false, false, false,
                        "preservation refused 123");
  std::ostringstream output;
  auto* previous = std::cout.rdbuf(output.rdbuf());
  counters.Print(0, Expected{});
  std::cout.rdbuf(previous);
  const auto rows = output.str();
  CHECK(rows.find("text=\"facet # cell # geometry refusal\" count=2") != std::string::npos);
  CHECK(rows.find("text=\"#x\" count=2") != std::string::npos);
  CHECK(rows.find("text=\"" + prefix + "\" count=2") != std::string::npos);
  CHECK(rows.find("text=\"preservation refused #\" count=1") != std::string::npos);
  CHECK(rows.find("guidance-only text") == std::string::npos);
  unsigned total = 0;
  std::istringstream lines(rows);
  std::string line;
  while (std::getline(lines, line)) {
    if (line.find("NATIVE3D_WALL_REASON ") != std::string::npos)
      total += std::stoul(line.substr(line.find("count=") + 6));
  }
  CHECK(total == 7);
}

TEST_CASE("Native wall frame timing preserves callback outcomes", "[NativeWallDiagnostics68]") {
  using SU2NativeWallDiagnostics3D::Counters;
  using SU2NativeWallDiagnostics3D::Expected;
  CHECK(SU2NativeTiming3D::Session::ReservedBytes() == 4120);
  Counters counters(true);
  int true_calls = 0, false_calls = 0, throw_calls = 0;
  CHECK(counters.TimeFrame("RefitWallSplit", [&] {
    ++true_calls;
    return true;
  }));
  CHECK_FALSE(counters.TimeFrame("ReconstructWallSplit", [&] {
    ++false_calls;
    return false;
  }));
  CHECK_THROWS_AS(counters.TimeFrame("ReconstructSplitPatch",
                                     [&]() -> bool {
                                       ++throw_calls;
                                       throw std::runtime_error("frame failure");
                                     }),
                  std::runtime_error);
  CHECK(true_calls == 1);
  CHECK(false_calls == 1);
  CHECK(throw_calls == 1);
  std::ostringstream output;
  auto* previous = std::cout.rdbuf(output.rdbuf());
  counters.Print(0, Expected{});
  std::cout.rdbuf(previous);
  const auto rows = output.str();
  CHECK(rows.find("operation=RefitWallSplit calls=1 valid=1 seconds=") != std::string::npos);
  CHECK(rows.find("operation=ReconstructWallSplit calls=1 valid=0 seconds=") != std::string::npos);
  CHECK(rows.find("operation=ReconstructSplitPatch calls=1 valid=0 seconds=") != std::string::npos);
  Counters disabled(false);
  int disabled_calls = 0;
  CHECK_FALSE(disabled.TimeFrame("RefitWallSplit", [&] {
    ++disabled_calls;
    return false;
  }));
  CHECK(disabled_calls == 1);
  std::ostringstream disabled_output;
  previous = std::cout.rdbuf(disabled_output.rdbuf());
  disabled.Print(0, Expected{});
  std::cout.rdbuf(previous);
  CHECK(disabled_output.str().empty());
}
