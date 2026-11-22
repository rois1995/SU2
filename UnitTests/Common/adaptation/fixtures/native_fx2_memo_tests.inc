/* SPDX-License-Identifier: LGPL-2.1-or-later */
#include "native_fx2_memo_report.hpp"
#include "../../../../Common/include/adaptation/CNativeTimerTree3D.hpp"

// The ordinary test target permits folding -0.0 into +0.0.
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC push_options
#pragma GCC optimize("no-fast-math", "no-unsafe-math-optimizations", "fp-contract=off")
#endif

namespace {
using namespace SU2Native3D;
using TimerSession = SU2NativeTiming3D::Session;
using TimerCalls = std::array<std::array<size_t, TimerSession::OperationCount>, TimerSession::PhaseCount>;

MappedFacetReference FX2Authority(double height = 1) {
  const Node a{0, {0, 0, 0}}, b{1, {1, 0, 0}}, c{2, {0, 1, 0}}, d{3, {0, 0, height}};
  const std::vector<Facet> facets{{0, 0, {a, c, b}}, {1, 0, {a, b, d}}, {2, 0, {a, d, c}}, {3, 0, {b, c, d}}};
  const std::map<Id, double> errors{{0, 1e-12}, {1, 1e-12}, {2, 1e-12}, {3, 1e-12}};
  return MappedFacetReference(std::make_shared<PlanarReference>(facets), errors, 57);
}

using FX2AuthorityHandle = std::shared_ptr<const MappedFacetReference>;

AtlasBudget FX2Budget() {
  auto budget = BackendBudget();
  budget.work = 17;
  budget.caller_bytes = 4096;
  budget.next_time_check = 11;
  return budget;
}

MobilityPolicy FX2Policy(const MappedFacetReference& authority, double angle, AtlasBudget& budget) {
  MobilityPolicy policy;
  std::string reason;
  const bool built = authority.BuildMobilityPolicy(angle, {{0, 2}}, policy, budget, reason);
  INFO(reason);
  REQUIRE(built);
  return policy;
}

MobilityPolicy FX2Policy(const MappedFacetReference& authority, double angle = 0.) {
  auto budget = FX2Budget();
  return FX2Policy(authority, angle, budget);
}

struct FX2Result {
  std::shared_ptr<BoundaryParameterState> state;
  std::string exception = "none", reason, row;
  TimerCalls calls{};
  size_t work = 0;
  bool peak_over_maximum = false;
};

template <class Call>
FX2Result FX2Observe(const char* name, const AtlasBudget* budget, Call call) {
  TimerSession timer(true, name, 0, 1, SU2NativeTiming3D::Phase::Construction);
  REQUIRE(timer.Owns());
  FX2Result result;
  try {
    result.state = call();
  } catch (const std::invalid_argument& error) {
    result.exception = "std::invalid_argument";
    result.reason = error.what();
  } catch (const std::length_error& error) {
    result.exception = "std::length_error";
    result.reason = error.what();
  }
  timer.Finish();
  const auto ledger = timer.Snapshot();
  for (size_t phase = 0; phase < TimerSession::PhaseCount; ++phase)
    for (size_t operation = 0; operation < TimerSession::OperationCount; ++operation)
      result.calls[phase][operation] = ledger.records[phase][operation].calls;
  CHECK(result.calls[static_cast<size_t>(SU2NativeTiming3D::Phase::Construction)]
                    [static_cast<size_t>(SU2NativeTiming3D::Operation::MobilityPolicy)] == 1);
  if (budget) {
    result.work = budget->work;
    result.peak_over_maximum = budget->peak_bytes > budget->maximum_bytes;
  }
  result.row = NativeFX2Fixture::CallRow(budget, result.exception.c_str(), result.reason, result.state.get());
  std::cout << "FX2_MEMO case=" << name << ' ' << result.row << '\n';
  return result;
}

FX2Result FX2Construct(const char* name, const FX2AuthorityHandle& authority, const MobilityPolicy& policy,
                       AtlasBudget budget = FX2Budget(), double tolerance = 0) {
  BoundaryPoints points;
  std::vector<SurfaceFace> surface;
  for (const auto& row : authority->Reference().Facets()) {
    const auto& f = row.second;
    surface.push_back({{f.v[0].id, f.v[1].id, f.v[2].id}, f.id});
    for (const auto& node : f.v) points.emplace(node.id, BoundaryPoint{node, authority->Vertex(node.id)});
  }
  return FX2Observe(name, &budget, [&] {
    return std::make_shared<BoundaryParameterState>(authority, points, surface, 1, SIZE_MAX, 0, BoundaryCoverage{},
                                                    policy, &budget, tolerance);
  });
}

void FX2Refused(const FX2Result& result) {
  CHECK_FALSE(result.state);
  CHECK(result.exception == "std::invalid_argument");
  CHECK(result.reason == "Boundary mobility policy differs from immutable geometry/configuration.");
}
}  // namespace

TEST_CASE("FX2 F1 compares accepted policy construction, identity keys and payload decoding", "[.][NativeFX2Memo]") {
  const auto authority = std::make_shared<MappedFacetReference>(FX2Authority());
  const auto identical = std::make_shared<MappedFacetReference>(FX2Authority());
  CHECK(authority->Curves().empty());
  CHECK_FALSE(authority->ReferenceComponents());
  const auto policy = FX2Policy(*authority);
  const auto check_resident = [&] { REQUIRE(authority->ResidentBytes() == identical->ResidentBytes()); };
  check_resident();
  const auto miss = FX2Construct("F1_first", authority, policy);
  REQUIRE(miss.state);
  check_resident();
  const auto hit = FX2Construct("F1_repeat", authority, policy);
  REQUIRE(hit.state);
  check_resident();
  const auto other = FX2Construct("F1_distinct", identical, policy);
  REQUIRE(other.state);
  CHECK(hit.row == miss.row);
  CHECK(hit.row == other.row);
  CHECK(hit.calls == miss.calls);
  CHECK(hit.calls == other.calls);

  constexpr uint64_t negative_zero_bits = uint64_t{1} << 63;
  // Read the bits at runtime: interprocedural constant propagation merges the two zero literals.
  const volatile uint64_t pinned_bits = negative_zero_bits;
  const uint64_t bits = pinned_bits;
  double negative_angle;
  std::memcpy(&negative_angle, &bits, sizeof(negative_angle));
  const auto negative_zero = FX2Policy(*authority, negative_angle);
  uint64_t observed_bits;
  std::memcpy(&observed_bits, &negative_zero.angle, sizeof(observed_bits));
  REQUIRE(observed_bits == negative_zero_bits);
  REQUIRE(EncodeBoundaryAtlas({}, negative_zero) != EncodeBoundaryAtlas({}, policy));
  check_resident();
  REQUIRE(FX2Construct("F1_negative_zero", authority, negative_zero).state);

  struct TwoAuthorities {
    MappedFacetReference x = FX2Authority();
    MappedFacetReference y = FX2Authority(1.25);
  };
  const auto owner = std::make_shared<TwoAuthorities>();
  const FX2AuthorityHandle x(owner, &owner->x), y(owner, &owner->y);
  const auto policy_x = FX2Policy(*x), policy_y = FX2Policy(*y);
  REQUIRE(EncodeBoundaryAtlas({}, policy_x) != EncodeBoundaryAtlas({}, policy_y));
  REQUIRE(FX2Construct("F1_alias_x", x, policy_x).state);
  FX2Refused(FX2Construct("F1_alias_y_wrong_policy", y, policy_x));
  REQUIRE(FX2Construct("F1_alias_y", y, policy_y).state);

  check_resident();
  const auto payload = hit.state->Payload();
  // FromPayload owns its reservation; only its result and timer ledger are observable through this API.
  const auto decoded = FX2Observe("F1_payload", nullptr, [&] { return BoundaryParameterState::FromPayload(authority, payload); });
  REQUIRE(decoded.state);
  CHECK(decoded.state->Snapshot() == hit.state->Snapshot());
  CHECK(decoded.state->Payload() == payload);
}

TEST_CASE("FX2 F2 refuses the same altered policy after a hit or a build", "[.][NativeFX2Memo]") {
  const auto authority = std::make_shared<MappedFacetReference>(FX2Authority());
  const auto cold = std::make_shared<MappedFacetReference>(FX2Authority());
  auto policy = FX2Policy(*authority);
  REQUIRE(FX2Construct("F2_record", authority, policy).state);
  REQUIRE_FALSE(policy.features.empty());
  policy.features.erase(policy.features.begin());
  const auto hit = FX2Construct("F2_hit_refusal", authority, policy);
  const auto miss = FX2Construct("F2_miss_refusal", cold, policy);
  FX2Refused(hit);
  FX2Refused(miss);
  CHECK(hit.row == miss.row);
  CHECK(hit.calls == miss.calls);
}

TEST_CASE("FX2 F3 preserves the budget charge and limiting failures", "[.][NativeFX2Memo]") {
  const auto authority = std::make_shared<MappedFacetReference>(FX2Authority());
  auto build = FX2Budget();
  const size_t initial_work = build.work;
  const auto policy = FX2Policy(*authority, 180, build);
  // This closed surface has no features at 180 degrees. Entity validation uses no further atlas work.
  REQUIRE(policy.features.empty());
  const double tolerance = 1e-4;
  const size_t work = build.work - initial_work, storage = build.peak_bytes - build.caller_bytes;
  REQUIRE(work > 0);
  REQUIRE(storage == (4 + 6) * 512);
  const auto construct = [&](const char* name, AtlasBudget budget) {
    return FX2Construct(name, authority, policy, budget, tolerance);
  };
  REQUIRE(construct("F3_record", FX2Budget()).state);

  auto short_work = FX2Budget();
  short_work.maximum_work = short_work.work + work - 1;
  FX2Refused(construct("F3_work_short", short_work));
  auto short_bytes = FX2Budget();
  short_bytes.maximum_bytes = short_bytes.caller_bytes + storage - 1;
  const auto refused_bytes = construct("F3_bytes_short", short_bytes);
  FX2Refused(refused_bytes);
  CHECK(refused_bytes.peak_over_maximum);

  auto deadline = FX2Budget();
  deadline.within_time = [] { return true; };
  REQUIRE(construct("F3_deadline_accepts", deadline).state);
  deadline.within_time = [] { return false; };
  FX2Refused(construct("F3_deadline_refuses", deadline));
  auto expired = FX2Budget();
  expired.time_exhausted = true;
  FX2Refused(construct("F3_time_exhausted", expired));
  auto exhausted = FX2Budget();
  exhausted.work_exhausted = true;
  FX2Refused(construct("F3_work_exhausted", exhausted));

  auto exact_work = FX2Budget();
  exact_work.maximum_work = exact_work.work + work;
  const auto work_result = construct("F3_work_equal", exact_work);
  REQUIRE(work_result.state);
  CHECK(work_result.work == exact_work.maximum_work);
  auto exact_bytes = FX2Budget();
  exact_bytes.maximum_bytes = exact_bytes.caller_bytes + storage;
  REQUIRE(construct("F3_bytes_equal", exact_bytes).state);
}

TEST_CASE("FX2 F4 an expired authority cannot qualify a new policy", "[.][NativeFX2Memo]") {
  MobilityPolicy old_policy;
  std::weak_ptr<const MappedFacetReference> expired;
  {
    const auto authority = std::make_shared<MappedFacetReference>(FX2Authority());
    expired = authority;
    old_policy = FX2Policy(*authority);
    REQUIRE(FX2Construct("F4_old", authority, old_policy).state);
  }
  REQUIRE(expired.expired());
  const auto changed = std::make_shared<MappedFacetReference>(FX2Authority(1.25));
  const auto changed_policy = FX2Policy(*changed);
  REQUIRE(EncodeBoundaryAtlas({}, changed_policy) != EncodeBoundaryAtlas({}, old_policy));
  REQUIRE(FX2Construct("F4_new", changed, changed_policy).state);
  FX2Refused(FX2Construct("F4_old_policy_refusal", changed, old_policy));
}

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC pop_options
#endif
