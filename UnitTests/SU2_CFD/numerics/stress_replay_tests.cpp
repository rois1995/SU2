#include "catch.hpp"
#include "../../../Common/include/toolboxes/stress_replay.hpp"
#include <filesystem>

namespace {
struct ReplayFixture {
  std::filesystem::path path =
      std::filesystem::temp_directory_path() /
      ("su2_replay_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  ~ReplayFixture() { std::filesystem::remove(path); }
  std::string Read() const {
    std::ifstream file(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(file), {}};
  }
  void Write(const std::string& value) { std::ofstream(path, std::ios::binary) << value; }
};
}  // namespace

TEST_CASE("Exact stress replay validates identities, records and complete use", "[stress][replay]") {
  ReplayFixture f;
  StressField::Tensor value{{{1, .2, 0}, {.2, -1, 0}, {0, 0, 0}}};
  StressField::Replay writer("identity", false, f.path.string());
  REQUIRE(writer.Exchange("face query", value) == value);
  REQUIRE(writer.Exchange("face query", value) == value);
  REQUIRE(writer.Exchange("source query", value) == value);
  writer.Finish(f.path.string());
  const auto original = f.Read();
  REQUIRE_THROWS(StressField::Replay("identity", false, f.path.string()));
  REQUIRE_THROWS(StressField::Replay("changed state", true, f.path.string()));
  StressField::Replay reader("identity", true, f.path.string());
  REQUIRE_THROWS(reader.Exchange("missing face", {}));
  REQUIRE(reader.Exchange("face query", {}) == value);
  REQUIRE_THROWS(reader.Finish(f.path.string()));
  REQUIRE(reader.Exchange("source query", {}) == value);
  REQUIRE_NOTHROW(reader.Finish(f.path.string()));
  f.Write(original + "corrupt");
  REQUIRE_THROWS(StressField::Replay("identity", true, f.path.string()));
  const auto third = original.find('\n', original.find('\n', original.find('\n') + 1) + 1);
  const auto payload = original.substr(third + 1);
  const auto duplicate = payload + payload;
  f.Write("SU2_STRESS_REPLAY_1 COUPLING_CONTROL_EXACT INTERNAL_UNITS\nidentity\n" + StressField::SHA256(duplicate) +
          "\n" + duplicate);
  REQUIRE_THROWS(StressField::Replay("identity", true, f.path.string()));
  value[0][0] = 2;
  REQUIRE_THROWS(StressField::Replay::Validate(value));
  value[0][0] = 1;
  value[1][0] = 0;
  REQUIRE_THROWS(StressField::Replay::Validate(value));
}

TEST_CASE("Field query counters distinguish locations and failures", "[stress][field]") {
  const std::string source = __FILE__;
  StressField::Field field(source.substr(0, source.find_last_of("/\\")) + "/../../fixtures/stress/positive.suf");
  field.Evaluate({.5, .5, 0}, 'F');
  field.Evaluate({.5, .5, 0}, 'S');
  REQUIRE_THROWS(field.Evaluate({2, 2, 0}, 'F'));
  const auto stats = field.QuerySummary();
  REQUIRE(stats.find("face_queries=2") != std::string::npos);
  REQUIRE(stats.find("source_queries=1") != std::string::npos);
  REQUIRE(stats.find("failed_queries=1") != std::string::npos);
}
