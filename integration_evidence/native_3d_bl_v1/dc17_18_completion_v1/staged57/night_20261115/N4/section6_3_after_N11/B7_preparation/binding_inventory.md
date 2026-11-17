# B7 phase and checkpoint binding inventory

Read-only inventory for the B7 EngineControl extraction. This corrected P-local copy supersedes the earlier draft at `N4/B7_preparation`; that draft used an obsolete, historical source check. No source or execution files were changed.

Snapshot: git `213fb7af052673cb9f5ef66b8b30e32faab66b10`; current `CNativeEngine3D.cpp` SHA256 `7545edb4cea518fffc7e0e5cd4f763891fbe2a002b2d26b177b04b76e0d3b848`; controlling P-local source-check SHA256 `3fff8559d3f67b42c1314f2388fb8ba9d510fa064e625f53ef99df0d7f17f8a1`.

## Source bindings

- Current phase/checkpoint lambdas are in `Common/src/adaptation/CNativeEngine3D.cpp:3261-3415`.
- `EngineControl` is the current owner of phase, checkpoint, timing-window, result, and progress slots (`Common/src/adaptation/native_engine3d/EngineControl.hpp:4-79`). `EngineContext` member order is Control, Census, Queues, Exchange, Commit, Ownership (`EngineContext.hpp:18-35`); preserve it. `EngineOwnership` separately owns `working` and ownership markers (`EngineOwnership.hpp:4-8`).
- The `Adapt` lambda bodies currently capture canonical state by reference. Keep the same working directory, Selection, stats, options, private parameters, source, World, queue/exchange services, and started-time instances. Do not turn any of these into copied function arguments or move checkpoint work across the round boundary.
- B7 cut-plan scope is the phase/checkpoint lambdas below. The plan's EngineControl table also lists round-head decisions B02/B07-B29, but those are Phase C1 and must stay in the loop for this B7 sub-step.

## Minimal method map (all current bodies fit the 80-line limit)

1. `begin_length_generation` (`3261-3276`, 16 lines): selection/memory admission; set surface lane and construction claim; update split lane/generation and optional history; begin directory construction; queue length direction; Selection phase reset; catch; one Vote; reset generation-window counters.
2. `surface_ready` (`3277-3280`, 4 lines): readiness is the collective OR of construction-invalid or incomplete census. Keep the exact single `world.maximum` and polarity.
3. `enter_phase` (`3281-3341`, 61 lines): surface-ready fast return or repair-entry path; otherwise switch construction/improvement state; close/open construction pass and queue direction; report construction entry at its old point; update handoff and phase counters; admit; optional length-generation start; ChangePhase/initial selection under its existing catch and Vote; run phase census; optional geometry-priority pass and its own admission/catch/Vote; phase ownership for repair; final observer. Preserve the two distinct entry paths and every short-circuit.
4. `discard_checkpoint` (`3342-3349`, 8 lines): retire directory stats, release/reset all checkpoint storage and flags. It has no explicit Vote or MPI collective in this body.
5. `capture_checkpoint` (`3350-3376`, 27 lines): strict census precondition comment; compute/admit memory; copy working cells and blocked state; collect canonical edges; snapshot optional parameters; construct dormant directory inside catch; Vote; build checkpoint directory; SUM-reduce cell count; record result-vector sizes; set valid/trial and increment stats.
6. `rollback_checkpoint` (`3377-3415`, 39 lines): verify noexcept swap contract and checkpoint/authority consistency; Vote; admit attempted-edge work; mark only attempted checkpoint edges in a catch; Vote; require a globally nonzero retired direction; swap private parameters, working, directory and blocked state; resize results and update rollback stats; discard; reset Selection/history/phase flags; re-enter Improvement.

No further split is required for these current individual lambda sizes. If extraction requires adapters, keep them narrowly scoped and do not split/reorder statements solely to reduce line count.

## Collective, timer, catch and state-lifetime ledger

- **`begin_length_generation`:** `Admit` precedes state changes; local mutations/catch precede `Vote`; window snapshots happen only after the vote. Preserve mutation order (`selection.lane`, `construction_claims`, stats/history, directory pass, queue direction, `ChangePhase`).
- **`surface_ready`:** `scope_census_complete` is inside the local expression passed to `world.maximum`; preserve short circuit and collective participation.
- **`enter_phase`:** the surface target fast path may return before the normal transition. Repair entry uses `LengthDirection`, ends the construction pass, changes Selection under a local catch, votes, resets windows, then observes. Normal entry includes an optional `world.maximum(improvement_changed)`, prior construction-entry observer, `world.maximum(construction_changed)`, state transition/window updates, admission, optional nested `begin_length_generation`, Selection/initial rebuild catch and Vote, `measure`, optional priority admission/catch/Vote, optional `phase_ownership(true)`, and final `observe_progress`. Do not merge or hoist the collectives across state changes.
- **`capture_checkpoint`:** `Admit` -> local snapshot/catch -> `Vote` -> dormant `Directory::Build` -> SUM allreduce -> checkpoint counters/flags. The checkpoint directory is built while its copied parameter state is active and dormant afterward. The plan's identity/lifetime warning makes this ordering material.
- **`rollback_checkpoint`:** validation Vote -> Admit -> attempted-edge restoration/catch -> Vote -> `world.maximum(retired!=0)` -> parameter/state swaps -> discard/reset -> `enter_phase(Improvement)`. Keep the proposal/import/private-parameter scopes ended before rollback; never allocate during the no-throw restoration swaps.
- No phase/checkpoint lambda creates a SU2 timer scope. Their accounting is via existing `stats.rounds`/phase-window values, `measure`, `observe_progress`, and explicit counters. Keep timer ownership and `started` in their original scope. Do not change repeated `finish()` returns in callers (`3515`, `3528`, `3543`, `3568`, `3575`, `3585`, `3597`, and later callsites).
- `discard_checkpoint` calls `RetireDirectory` but has no local Vote. Do not infer or add one. `Directory` methods and `Admit`/`Vote` internals keep their existing contracts.

## Consolidated findings / source caveats (corrected against the controlling P source check)

1. **Historical source-check correction:** the earlier `N4/source_check.md` is not controlling and its F3b/P7/P8-absent statement is superseded. The controlling whole-item check is `P/source_check.md` (SHA256 recorded above): P7/P8 were accepted at `429c6c1a60`; the B7 cache attempt failed its memory result gate and was reverted at `9831c787e7`; the latest accepted cut is `76b2ad61db`. The last fully qualified workflow is `5ec465ca1a`. CUT_PLAN's “after P7 / P8 and B7” reference phrase is stale for B7 and must not be used as acceptance provenance. Current progress says N14 full552 session 79066 is running; this inventory does not treat its result as terminal or accepted.
2. **PART1 snapshot line drift:** the saved PART1 coordinates are for an earlier snapshot. Current B7 is at `3261-3415`; find names/call sites from the live source instead of applying old line offsets. P/source_check also says PART1 is an older working snapshot; follow current names, declarations, and ordering.
3. **Phase C separation:** EngineControl's table entry also names round-head decisions B02/B07-B29, but the B7 ordered step is phases/checkpoints; do not move round-loop decisions into this sub-step. They remain for C1 and its separate gates.
4. Current ordering and captures agree with the controlling P source-check constraints: state objects are non-copyable/non-movable; referenced canonical objects and RAII destruction order must remain fixed. No additional source/result contradiction was found in this read.

## Call-site map

- Nested: `enter_phase` -> `surface_ready`, `begin_length_generation`, `phase_ownership`, measurement and observer.
- Nested: `rollback_checkpoint` -> `discard_checkpoint` -> `enter_phase(Improvement)`.
- Loop callers: `begin_length_generation` at `3515`, `3663`; `enter_phase` at `3528`, `3543`, `3585`, `3597`, `3667`, `3692`, `3745`; checkpoint capture at `3585`, rollback at `3568`/`3575`, discard at `3579`. These `if (!...) return finish()` branches are part of the acceptance-preserving binding.

## Source paths reviewed

- `P/source_check.md` (controlling whole-item source check; SHA256 `3fff8559d3f67b42c1314f2388fb8ba9d510fa064e625f53ef99df0d7f17f8a1`)
- `N4/section6_3_after_N11/inputs/CUT_PLAN_8_2.md`, `inputs/CUT_INVENTORY_PART1.md`
- `N14/section6_12/inputs/CUT_PLAN_8_2.md` (B7 wording)
- `CNativeEngine3D.cpp`, `EngineControl.hpp`, `EngineOwnership.hpp`, `EngineContext.hpp`
