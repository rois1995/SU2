# B6 `phase_ownership` proposal status

## Fresh binding

The inventory's former 552 snapshot (`CNativeEngine3D.cpp` SHA
`7545edb4cea518fffc7e0e5cd4f763891fbe2a002b2d26b177b04b76e0d3b848`) is no
longer current. The candidate B3 main source now has SHA
`806e13eccc41df3a6e19602b38e1297dc8b9852fd4a6a8804af7a8b67d4d80e5`.
Current `EngineOwnership.hpp` is
`474b4ef2d06f079202bd9ece9e01165ed48dab93c4cc23b72ef64a570e8a9e81` and
`EngineOwnership.cpp` is
`064e1b42d50255852194f4eb7bfa54e8036d5f4c1f2bc21d09744c8b8ea266d5`.
The controlling source check is the P-local
`source_check.md`, SHA
`3fff8559d3f67b42c1314f2388fb8ba9d510fa064e625f53ef99df0d7f17f8a1`; plan
and PART1 inputs are under `../inputs/`. The source currently locates
`phase_ownership` at line 2874 and call sites at 3157, 3489, and 3559. Do not
use the old line positions in the earlier inventory as bindings for generated
source.

## Extraction map for the current candidate

The seven logical boundaries in `../binding_inventory.md` still correspond to
the current lambda body. Proposed private member boundaries are:

1. `PhaseOwnershipGate(context, quality)` — deadline, boundary/evaluated-once
   gates, checkpoint/census/repair-zero/progress checks and global cell count.
2. `PhaseOwnershipAdmission(context, quality, scratch, complete_stars)` —
   admission, star census and its following deadline check.
3. `PhaseOwnershipCostModel(context, quality, complete_stars, failure, costs,
   geometry_costs, weights, geometric, quality_debts, local sums)` — exact
   existing catch body and no extra edge/geometry queries.
4. `PhaseOwnershipDecision(context, quality, vectors, local sums, transition
   start)` — same reductions, weights, cost/gain calculation, world maximum,
   report, and no-migration accounting.
5. `PhaseOwnershipStage(...)` — pending ownership/selection/directory
   construction, first Vote, repartition and timing checks.
6. `PhaseOwnershipMigration(...)` — outgoing certificate construction, Vote,
   exchange, admission and deadline checks.
7. `PhaseOwnershipRestoreCommit(...)` — directory build, restore/census,
   Vote, staged observation, identity swaps/cache resets, counter report, and
   final observer.

The coordinator must keep all cross-boundary state as its current local objects
and pass references. In particular, the single `CLocalFailure` lifetime spans
the cost, pending allocation, outgoing certificate, and restore catches/votes;
do not recreate or clear it between helpers. Vectors (`costs`, `geometry_costs`,
`weights`, `geometric`, `quality_debts`) and `complete_stars` remain the same
containers from their current declarations through their last uses. Pending
objects remain created at the staging point and alive through final commit.

## Binding hazards before code generation

- `observe_progress` is still an Adapt-local lambda in this candidate; B5 has
  not been applied. Staged and committed observer calls must retain the existing
  synchronous callable by reference in a template helper, or be rebound after
  B5 to the resulting `EngineControl` member. No stored callback, wrapper
  object, or extra callback invocation is acceptable.
- All frequently referenced names are live aliases owned by `EngineContext` or
  its sub-owners: `world`, `options`, `working`, `selection`, `stats/result`,
  `geometry`, `directory/source`, `checkpoint`, repair/coarsening state,
  selection/census/queue caches, counters, and initial/seed state. Any generator
  must audit each name against current headers and preserve reference
  constness; do not add a copy to simplify signatures.
- `within_time` is a local closure over the existing clock and result state.
  Keep it within the coordinator or pass the existing closure synchronously by
  reference. Do not introduce an additional time check or move one across an
  exchange/vote.
- The original `phase_ownership` call results are immediately tested and
  callers return through `finish()` on failure. Preserve those exact caller
  short-circuits and the ownership boundary timing.
- The proposal is not yet a generated extraction. No actual-body token proof,
  capture audit, or byte reversal is claimed by this status artifact. Generate
  only after current candidate B3/B5 status is confirmed, and require a script
  to extract the actual current lambda, inline all helper calls, compare token
  streams, and reverse the caller edits byte-for-byte before considering it
  reviewable.

No production file was edited; no build, test, or numerical job was run.
