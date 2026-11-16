/* I.B0: worker-local observations of existing wall decisions. Inputs are completed checks;
 * outputs are diagnostic rows only. Fixed storage, no geometry query or acceptance side effect. */
#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <string_view>

namespace SU2NativeWallDiagnostics3D {

// Counts worker-local observations only; never reads counters to control mesh edits.
enum class PatchSource : std::uint8_t {
  Primary,
  RefitWallSplit,
  ReconstructWallSplit,
  ReconstructSplitPatch,
  Unreported
};

enum class GuardOrigin : std::uint8_t { Operator, Guidance, Count };
enum class GuidanceCause : std::uint8_t { Incomplete, Nonfinite, NoProgress, AfterAbove, Exception, Other, Count };
enum class PreTrialDisposition : std::uint8_t { Other, Oversized, Conflict, Deferred, SplitPosition, Count };

// Unmodified engine totals supplied at exit; used only to report reconciliation.
struct Expected {
  std::uint64_t geometry_trials = 0;
  std::uint64_t geometry_rejected = 0;
  std::uint64_t guidance_trials = 0;
  std::uint64_t guidance_rejected = 0;
  std::uint64_t accepted = 0;
  std::uint64_t metric_trials = 0;
  std::uint64_t proposed = 0;
  std::uint64_t attempted = 0;
};

// One Adapt worker owns these fixed counters; no counter value participates in an edit decision.
class Counters {
 public:
  // Enabling changes observations only; all observed Boolean results are returned unchanged.
  explicit Counters(bool enabled) : enabled_(enabled) {}

  // Mirrors one existing geometry_trial call; it does not classify its caller.
  void ObserveGeometry(bool valid) {
    if (!enabled_) return;
    ++geometry_trials_;
    geometry_rejected_ += !valid;
  }

  // Records only the first false predicate in guarded_geometry's existing order.
  void ObserveGuard(std::string_view operation, GuardOrigin origin, bool initially_valid, bool preserved,
                    bool height_valid, bool boundary_valid, std::string_view reason) {
    if (!enabled_ || (initially_valid && preserved && height_valid && boundary_valid)) return;
    if (origin == GuardOrigin::Guidance) {
      ++guidance_guard_refusals_;
      return;
    }
    ObserveOperatorReason(reason);
    if (!initially_valid) {
      ++operator_refusals_[OperatorClass(operation)][Reason(reason)];
    } else if (!preserved) {
      ++preserve_refusals_;
    } else if (!height_valid) {
      ++height_refusals_;
    } else {
      ++boundary_refusals_;
    }
  }

  // Measures one raw frame-operator invocation when enabled and returns its unchanged result.
  template <class Callback>
  bool TimeFrame(std::string_view operation, Callback&& callback) {
    const auto index = FrameIndex(operation);
    if (!enabled_ || index == frame_calls_.size()) return static_cast<bool>(callback());
    ++frame_calls_[index];
    const auto start = std::chrono::steady_clock::now();
    try {
      const bool valid = static_cast<bool>(callback());
      RecordFrame(index, start, valid);
      return valid;
    } catch (...) {
      RecordFrame(index, start, false);
      throw;
    }
  }

  // Observes one geometric_change call; geometric identifies its geometry-versus-size queue origin.
  void GuidanceTrial(bool geometric) {
    if (!enabled_) return;
    ++guidance_trials_;
    geometry_queue_guidance_trials_ += geometric;
  }

  // Cause precedence is assigned by the caller; this method records the supplied result.
  void GuidanceReject(GuidanceCause cause, bool geometric) {
    if (!enabled_) return;
    ++guidance_rejected_;
    ++guidance_rejections_[static_cast<std::size_t>(geometric)][static_cast<std::size_t>(cause)];
  }

  // Counts validation refusals while returning the exact input decision.
  bool MetricValidation(bool valid) {
    if (enabled_) {
      ++metric_validation_trials_;
      metric_validation_rejected_ += !valid;
    }
    return valid;
  }

  // `before && !after` is the disjoint true-to-false metric_trial veto subset.
  void MetricTrial(bool before, bool after) {
    if (!enabled_) return;
    ++metric_trials_;
    metric_rejected_ += !after;
    metric_vetoed_ += before && !after;
  }

  // Mirrors one existing accepted increment after its successful publication.
  void Accepted() {
    if (enabled_) ++accepted_;
  }

  // Counts face encounters and committed split origins without feeding selection.
  void ObserveLawFace(bool weighted) {
    if (!enabled_) return;
    ++law_faces_[static_cast<std::size_t>(weighted)];
  }

  void ObserveSplit(bool geometric) {
    if (enabled_) ++accepted_splits_[static_cast<std::size_t>(geometric)];
  }

  // False collective probes and derived repair patches have no raw source to report.
  void ObserveSource(PatchSource source, bool available, bool guidance_rejected, bool approved) {
    const auto index = static_cast<std::size_t>(source);
    if (!enabled_ || !available || index >= sources_.size()) return;
    auto& row = sources_[index];
    ++row.validated;
    row.guidance_rejected += guidance_rejected;
    row.approved += approved;
  }

  // Kept outside the selected-choice partition: these calls need not name a choice.
  void PlanningRefusal() {
    if (enabled_) ++planning_refusals_;
  }

  // Prints observational totals and arithmetic residuals; diagnostics never reject work.
  void Print(int rank, const Expected& expected) const noexcept {
    if (!enabled_) return;
    try {
      constexpr const char* operators[] = {"SplitEdge",      "InsertEdgeCavity",     "other_geometry",
                                           "RefitWallSplit", "ReconstructWallSplit", "ReconstructSplitPatch"};
      constexpr const char* reasons[] = {
          "orientation", "interface", "surface_containment", "feature", "marker_junction", "incomplete_star", "other"};
      constexpr const char* causes[] = {"incomplete", "nonfinite", "no_progress", "after_above", "exception", "other"};
      constexpr const char* origins[] = {"size", "geometry"};
      constexpr const char* dispositions[] = {"other", "oversized", "conflict", "deferred", "split_position"};
      for (std::size_t op = 0; op < operator_refusals_.size(); ++op)
        for (std::size_t why = 0; why < 7; ++why)
          std::cout << "NATIVE3D_WALL_REFUSAL rank=" << rank << " class=" << operators[op] << " reason=" << reasons[why]
                    << " count=" << operator_refusals_[op][why] << '\n';
      PrintOperatorReasons(rank);
      PrintFrames(rank);
      PrintLawSources(rank);
      PrintSingle(rank, "PreservePassingFirstHeights", preserve_refusals_);
      PrintSingle(rank, "height_check", height_refusals_);
      PrintSingle(rank, "boundary_replacement_ready", boundary_refusals_);
      PrintSingle(rank, "guidance_guard", guidance_guard_refusals_);
      const auto guarded_refusals = GuardRefusals();
      PrintSingle(rank, "unwrapped_geometry_trial",
                  geometry_rejected_ >= guarded_refusals ? geometry_rejected_ - guarded_refusals : 0);
      for (std::size_t origin = 0; origin < 2; ++origin)
        for (std::size_t cause = 0; cause < 6; ++cause)
          std::cout << "NATIVE3D_WALL_GUIDANCE rank=" << rank << " origin=" << origins[origin]
                    << " cause=" << causes[cause] << " count=" << guidance_rejections_[origin][cause] << '\n';
      for (std::size_t why = 0; why < dispositions_.size(); ++why)
        std::cout << "NATIVE3D_WALL_PRETRIAL rank=" << rank << " disposition=" << dispositions[why]
                  << " count=" << dispositions_[why] << '\n';
      const auto priority =
          expected.guidance_trials >= guidance_trials_ ? expected.guidance_trials - guidance_trials_ : 0;
      const auto unwrapped = geometry_rejected_ >= guarded_refusals ? geometry_rejected_ - guarded_refusals : 0;
      const bool consistent =
          geometry_trials_ == expected.geometry_trials && geometry_rejected_ == expected.geometry_rejected &&
          geometry_rejected_ >= guarded_refusals && expected.guidance_trials >= guidance_trials_ &&
          guidance_rejected_ == expected.guidance_rejected && accepted_ == expected.accepted &&
          metric_trials_ == expected.metric_trials && selected_ == operator_entered_ + PreTrialTotal();
      std::cout << "NATIVE3D_WALL_COUNTS rank=" << rank << " geometry_trials=" << geometry_trials_
                << " expected_geometry_trials=" << expected.geometry_trials
                << " geometry_rejected=" << geometry_rejected_
                << " expected_geometry_rejected=" << expected.geometry_rejected
                << " guidance_trials=" << guidance_trials_ + priority
                << " expected_guidance_trials=" << expected.guidance_trials
                << " geometric_change_guidance_trials=" << guidance_trials_ << " priority_guidance=" << priority
                << " geometry_queue_guidance_trials=" << geometry_queue_guidance_trials_
                << " size_queue_guidance_trials=" << guidance_trials_ - geometry_queue_guidance_trials_
                << " guidance_rejected=" << guidance_rejected_
                << " expected_guidance_rejected=" << expected.guidance_rejected << " accepted=" << accepted_
                << " expected_accepted=" << expected.accepted << " metric_trials=" << metric_trials_
                << " expected_metric_trials=" << expected.metric_trials
                << " metric_validation_trials=" << metric_validation_trials_
                << " metric_validation_rejected=" << metric_validation_rejected_
                << " metric_rejected=" << metric_rejected_ << " metric_vetoed=" << metric_vetoed_
                << " selected=" << selected_ << " operator_entered=" << operator_entered_
                << " pretrial=" << PreTrialTotal() << " choice_partition_sum=" << operator_entered_ + PreTrialTotal()
                << " expected_proposed=" << expected.proposed << " expected_attempted=" << expected.attempted
                << " planning_refusals=" << planning_refusals_ << " guarded_refusals=" << guarded_refusals
                << " unwrapped_geometry_trials=" << unwrapped
                << " refusal_partition_sum=" << guarded_refusals + unwrapped
                << " counter_sums_match=" << unsigned(consistent) << '\n';
    } catch (...) {
    }
  }

 private:
  struct SourceCounts {
    std::uint64_t validated = 0, guidance_rejected = 0, approved = 0;
  };

  void PrintLawSources(int rank) const {
    constexpr const char* names[] = {"primary", "RefitWallSplit", "ReconstructWallSplit", "ReconstructSplitPatch"};
    for (std::size_t i = 0; i < sources_.size(); ++i) {
      const auto& row = sources_[i];
      std::cout << "NATIVE3D_WALL_SOURCE rank=" << rank << " source=" << names[i] << " validated=" << row.validated
                << " guidance_rejected=" << row.guidance_rejected << " approved=" << row.approved << '\n';
    }
    std::cout << "NATIVE3D_WALL_LAW_SELECTION rank=" << rank << " law_weighted_faces=" << law_faces_[1]
              << " law_fallback_faces=" << law_faces_[0] << " accepted_geometry_splits=" << accepted_splits_[1]
              << " accepted_size_splits=" << accepted_splits_[0] << '\n';
  }

  struct OperatorReason {
    std::array<char, 97> text{};
    std::uint64_t count = 0;
  };

  static std::size_t OperatorClass(std::string_view operation) {
    if (operation == "SplitEdge") return 0;
    if (operation == "InsertEdgeCavity") return 1;
    if (operation == "RefitWallSplit") return 3;
    if (operation == "ReconstructWallSplit") return 4;
    if (operation == "ReconstructSplitPatch") return 5;
    return 2;
  }

  static std::size_t FrameIndex(std::string_view operation) {
    if (operation == "RefitWallSplit") return 0;
    if (operation == "ReconstructWallSplit") return 1;
    if (operation == "ReconstructSplitPatch") return 2;
    return 3;
  }

  void RecordFrame(std::size_t index, std::chrono::steady_clock::time_point start, bool valid) {
    frame_seconds_[index] += std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    frame_valid_[index] += valid;
  }

  // Emits one fixed-storage observation row for each named frame operation.
  void PrintFrames(int rank) const {
    constexpr const char* names[] = {"RefitWallSplit", "ReconstructWallSplit", "ReconstructSplitPatch"};
    for (std::size_t i = 0; i < frame_calls_.size(); ++i)
      std::cout << "NATIVE3D_WALL_FRAME rank=" << rank << " operation=" << names[i] << " calls=" << frame_calls_[i]
                << " valid=" << frame_valid_[i] << " seconds=" << frame_seconds_[i] << '\n';
  }

  // Buckets reasons by the exact prefix that the bounded report can display.
  void ObserveOperatorReason(std::string_view reason) {
    std::array<char, 97> normalized{};
    std::size_t size = 0;
    bool previous_digit = false;
    for (const char value : reason.substr(0, reason.size() < 96 ? reason.size() : 96)) {
      const bool digit = value >= '0' && value <= '9';
      if (digit) {
        if (!previous_digit) normalized[size++] = '#';
      } else {
        normalized[size++] = value;
      }
      previous_digit = digit;
    }
    const auto prefix = std::string_view(normalized.data(), size);
    for (auto& entry : operator_reason_texts_) {
      if (entry.count && std::string_view(entry.text.data()) == prefix) {
        ++entry.count;
        return;
      }
    }
    for (auto& entry : operator_reason_texts_) {
      if (entry.count) continue;
      for (std::size_t i = 0; i < size; ++i) entry.text[i] = prefix[i];
      entry.text[size] = '\0';
      entry.count = 1;
      return;
    }
    ++operator_reason_more_;
  }

  // Emits one bounded quoted field with line-safe escapes.
  static void PrintQuoted(std::string_view text) {
    std::cout << '"';
    for (unsigned char value : text) {
      if (value == '"' || value == '\\')
        std::cout << '\\' << char(value);
      else if (value == '\n')
        std::cout << "\\n";
      else if (value == '\r')
        std::cout << "\\r";
      else if (value == '\t')
        std::cout << "\\t";
      else if (value < 0x20)
        std::cout << '?';
      else
        std::cout << char(value);
    }
    std::cout << '"';
  }

  // Reports retained display-prefix buckets for operator refusal reasons.
  void PrintOperatorReasons(int rank) const {
    for (const auto& entry : operator_reason_texts_) {
      if (!entry.count) continue;
      std::cout << "NATIVE3D_WALL_REASON rank=" << rank << " text=";
      PrintQuoted(std::string_view(entry.text.data()));
      std::cout << " count=" << entry.count << '\n';
    }
    if (operator_reason_more_) {
      std::cout << "NATIVE3D_WALL_REASON rank=" << rank << " text=\"<more>\" count=" << operator_reason_more_ << '\n';
    }
  }

  // Reads the complete reason text without allocating or changing it.
  static bool Contains(std::string_view value, std::string_view needle) {
    return value.find(needle) != std::string_view::npos;
  }

  // Reason matching is bounded to a fixed ordered set; unknown messages map to `other`.
  static std::size_t Reason(std::string_view reason) {
    if (Contains(reason, "marker junction") || Contains(reason, "marker-junction")) return 4;
    if (Contains(reason, "orientation") || Contains(reason, "invert") || Contains(reason, "nonpositive") ||
        Contains(reason, "non-positive") || Contains(reason, "degenerate") || Contains(reason, "cannot see"))
      return 0;
    if (Contains(reason, "interface")) return 1;
    if (Contains(reason, "contain") || Contains(reason, "finite chart") || Contains(reason, "same-chart")) return 2;
    if (Contains(reason, "feature") || Contains(reason, "corner") || Contains(reason, "crease") ||
        Contains(reason, "trace"))
      return 3;
    if (Contains(reason, "incomplete") || Contains(reason, "star")) return 5;
    return 6;
  }

  // Emits one disjoint guard-refusal class for this rank.
  void PrintSingle(int rank, const char* name, std::uint64_t count) const {
    std::cout << "NATIVE3D_WALL_REFUSAL rank=" << rank << " class=" << name << " count=" << count << '\n';
  }

  // Sums guarded failures; raw geometry_trial residuals are reported separately.
  std::uint64_t GuardRefusals() const {
    std::uint64_t count = preserve_refusals_ + height_refusals_ + boundary_refusals_ + guidance_guard_refusals_;
    for (const auto& row : operator_refusals_)
      for (auto value : row) count += value;
    return count;
  }

  // Sums the selected choices that have not entered the reconstruction body.
  std::uint64_t PreTrialTotal() const {
    std::uint64_t count = 0;
    for (auto value : dispositions_) count += value;
    return count;
  }

  friend class Choice;
  std::array<SourceCounts, 4> sources_{};
  std::array<std::uint64_t, 2> law_faces_{}, accepted_splits_{};
  bool enabled_;
  std::uint64_t geometry_trials_ = 0, geometry_rejected_ = 0;
  std::uint64_t guidance_trials_ = 0, guidance_rejected_ = 0, geometry_queue_guidance_trials_ = 0;
  std::uint64_t preserve_refusals_ = 0, height_refusals_ = 0, boundary_refusals_ = 0, guidance_guard_refusals_ = 0;
  std::uint64_t metric_validation_trials_ = 0, metric_validation_rejected_ = 0;
  std::uint64_t metric_trials_ = 0, metric_rejected_ = 0, metric_vetoed_ = 0;
  std::uint64_t accepted_ = 0, planning_refusals_ = 0, selected_ = 0, operator_entered_ = 0;
  std::array<std::array<std::uint64_t, 7>, 6> operator_refusals_{};
  std::array<OperatorReason, 16> operator_reason_texts_{};
  std::uint64_t operator_reason_more_ = 0;
  std::array<std::uint64_t, 3> frame_calls_{}, frame_valid_{};
  std::array<double, 3> frame_seconds_{};
  std::array<std::array<std::uint64_t, 6>, 2> guidance_rejections_{};
  std::array<std::uint64_t, static_cast<std::size_t>(PreTrialDisposition::Count)> dispositions_{};
};

// Eager bookkeeping keeps early finish() reports complete, without destructor timing effects.
class Choice {
 public:
  // Observes one slot; Begin receives its existing elected-active flag.
  explicit Choice(Counters& counters) : counters_(counters) {}

  // Starts one elected active slot in its explicit default pre-trial bucket.
  void Begin(bool active) {
    active_ = active;
    entered_ = false;
    disposition_ = PreTrialDisposition::Other;
    if (active_ && counters_.enabled_) {
      ++counters_.selected_;
      ++counters_.dispositions_[static_cast<std::size_t>(disposition_)];
    }
  }

  // Moves this slot exactly once from its pre-trial bucket into operator entry.
  void EnterOperator() {
    if (!active_ || entered_) return;
    entered_ = true;
    if (counters_.enabled_) {
      --counters_.dispositions_[static_cast<std::size_t>(disposition_)];
      ++counters_.operator_entered_;
    }
  }

  // Replaces the default/previous disposition while the slot has not entered an operator.
  void Disposition(PreTrialDisposition disposition) {
    if (!active_ || entered_ || disposition >= PreTrialDisposition::Count) return;
    if (counters_.enabled_) {
      --counters_.dispositions_[static_cast<std::size_t>(disposition_)];
      ++counters_.dispositions_[static_cast<std::size_t>(disposition)];
    }
    disposition_ = disposition;
  }

 private:
  Counters& counters_;
  PreTrialDisposition disposition_ = PreTrialDisposition::Other;
  bool active_ = false, entered_ = false;
};

}  // namespace SU2NativeWallDiagnostics3D
