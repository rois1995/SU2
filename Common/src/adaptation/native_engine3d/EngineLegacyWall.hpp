// Sequences the unchanged joint wall searches using the caller's private trial state.
#pragma once

struct EngineContext;
struct RoundState;
struct SlotState;
struct TrialChecks;
struct TrialComm;

struct EngineLegacyWall {
  template <class Reserve, class BoundaryReady, class Observe>
  static bool Run(EngineContext& context, RoundState& round_state, SlotState& slot_state, TrialComm& trial_comm,
                  TrialChecks& trial_checks, Reserve& reserve_dependencies, BoundaryReady& boundary_replacement_ready,
                  Observe& observe_transaction, bool split_failed, PatchMetric& before,
                  std::map<Edge, EdgeMeasure>& reuse, const std::vector<Cell>& empty);
};
