// Owns round DATA slots only; indices remain direct for-loop values.
#pragma once

struct RoundState {
  size_t round = 0;
  size_t batch = 0;       // Agreed width of the current round.
  size_t token_base = 0;  // Sum of rank count times width for all preceding rounds.
  std::optional<unsigned> timed_lane;
  std::optional<double> untimed_lane;
  std::optional<Choice> choice;
  std::optional<bool> complete;
  std::optional<bool> any;
  std::optional<bool> bad_field;
  std::optional<bool> refill;
  std::optional<bool> clean_quality;
  std::optional<bool> concurrent;
  std::optional<bool> batched;
  std::optional<std::vector<Choice>> choices;
  std::optional<DependencyBatch> dependency_batch;
  std::optional<ImportedBatch> imported_batch;
  std::optional<ReservationBatch> reserved;
  std::optional<size_t> slots;
  std::optional<bool> planned_coarsening;
  std::optional<size_t> refused_candidates;
  std::optional<size_t> refused_slots;
  std::optional<size_t> pass_deferred_candidates;
  std::optional<std::array<RepairReceipt, MaximumRoundBatch>> refused_receipts;
  std::optional<size_t> refused_receipt_count;
  std::optional<std::vector<RepairProposal>> repair_proposals;
  std::optional<CLocalFailure> private_failure;
  std::optional<uint64_t> private_work_used;
  std::optional<uint64_t> private_work_reserved;
  std::optional<bool> private_deadline;
  std::optional<std::array<lane_census::Slot, MaximumRoundBatch>> lane_slots;
  std::optional<Clock::time_point> private_started;
  std::optional<uint64_t> private_collectives;

  RoundState();
  ~RoundState();
  RoundState(const RoundState&) = delete;
  RoundState& operator=(const RoundState&) = delete;
  RoundState(RoundState&&) = delete;
  RoundState& operator=(RoundState&&) = delete;
};
