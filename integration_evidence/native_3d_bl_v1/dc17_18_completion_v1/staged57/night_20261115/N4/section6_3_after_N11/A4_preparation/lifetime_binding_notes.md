# A4 round/slot lifetime and binding notes

Read-only companion to `current_round_slot_field_manifest.md`. The observed tree is the current A3 Exchange candidate and is validation pending. PART2 is the block map, not a current line-number authority. No code or helper was changed.

## Binding and scope constraints

- `RoundState` / `SlotState` are inventory names in `CUT_INVENTORY_PART2.md`, not current declared structs. Round control and candidate locals are interleaved across the round body, not a contiguous declaration bundle.
- Many locals are referenced by `[&]` lambdas (`pick`, `refuse`, `plan_choices`, `reserve_dependencies`, the trial switch lambdas, `note_ladder`, `observe_transaction`, `boundary_replacement_ready`, and `evaluate_candidate`). Names can shadow, and later branches expect the same bindings. In particular round `complete` is distinct from `complete` in repair/census blocks; textual name equality does not mean one variable.
- The source has no named round/slot state object. `Choice choice` is round-local but overwritten in a slot; `Dependencies`/`Imported` and the candidate patch state are slot-local. `repair_proposals` receives moved values in private repair and is then consumed in the separate batch commit. `lane_slots` is fixed at eight and survives to that commit.
- Compile-time/bound constants remain separate: `MaximumCandidateBatch` is 8; `MaximumReplacementCells`, `MaximumCavityCells`, and related constants govern the ID offsets and scratch estimates. `epoch`, `token`, `offset`, `construction_nodes`, `node_stride`, `node_offset` are per-slot `const` locals derived from `round`, rank, slot, options, and candidate mode; they are not aliases or independent mutable state.
- `result.stats` is referenced as `stats` in the enclosing function; `working`, `selection`, `directory`, `source`, `geometry`, `world`, `options`, `atlas_budget`, and the batch bookkeeping belong to the enclosing engine context/locals. They are canonical shared state used by these scopes, not duplicated round fields. PART2 specifically notes function-level coupling state at §2.1.

## RAII objects that remain distinct

- `phase_scope` (3625) labels a whole round's timing phase. `lane_timer` (3643) is a whole-round surface/interior timer. Both are independent of per-operation `Timer` objects and are destroyed as round scope exits.
- `Timer` instances wrap narrow scheduling, dependency, reservation, routing, publication and update blocks. Some `continue` statements occur inside timer scopes; normal C++ destruction occurs on that edge. Do not fold those timers into a state aggregate with a longer lifetime.
- `wall_diagnostics::Choice wall_choice` begins/disposes one chosen candidate's diagnostic record. It is not the same state as `lane_census::Slot` or `Choice`.
- `CavityScope` begins immediately after `source.SetCavity(&imported.cells)` and lives until the slot body ends. It is deliberately longer than `evaluate_candidate()`; `source` continues to need the imported cavity during subsequent boundary checks/commit. Every ordinary slot exit, `continue`, early `return finish()`, or stack unwinding invokes its destructor.
- `lane_census::Trial` begins only when `evaluate_candidate()` is entered, at that lambda's first statement. It covers the candidate evaluation call and measures `repair_work` delta; its destructor runs as that lambda returns/unwinds, before caller-side `private_repair=false` in the private path. It does not time the later publication/commit. Slot metadata persists in `lane_slots` for commit reporting.
- Each `FieldAccounting{stats, field}` is a smaller nested RAII scope around field-query work (definition at 1315–1319; candidate uses include 4599, 5089, 5208, 5598, 5807, 5917). Its destructor calls `FieldCounts(stats, field)`, including when its nested operation unwinds. Instances are deliberately beside target creation/query batches and must not be lengthened or merged with candidate state.
- `D9QueryScope`, `CpuTimer`, and geometry/timing RAII wrappers similarly scope a particular query/operator. They are not candidate payloads.

## Collective, short-circuit and early-finish dependencies

- Before the slot loop, batched selection computes a common `slots` count with `TimedAllreduce(MAX)`. Non-repair ranks may have an inactive slot but still follow the common loop. Reservation, `world.maximum`, `vote`, service-frontier and publication calls have rank-synchronization requirements; a local early `continue` is only safe at the specific branch where source already arranges rank agreement or private local mode.
- Private repair has an explicit local-evaluation interval. The loop sets `private_repair=true`, evaluates candidate and coverage work, restores it to false, adds `repair_work`, then stores/moves proposal state. It intentionally avoids collectives while evaluating each slot; the `private_collectives` snapshot is checked later. The repair proposal's contact/reservation/publication path is collective after the local slot loop.
- Special surface-collapse evaluation similarly toggles `private_repair` around `evaluate_candidate`, checks the world collective counters and global work are unchanged, restores the mode, then calls `vote(local)`. The ordinary path calls `evaluate_candidate` directly and may finish immediately on false.
- `finish` is an enclosing lambda and may be invoked from inside round/slot scopes. It begins with `vote(CLocalFailure{})`, then does final proof/observation and result finalization. Calling it does not first unwind the current slot's RAII objects; they unwind only as the return expression/stack exits the enclosing scopes. This matters for `CavityScope`, `Timer`, `FieldAccounting`, and lane Trial cleanup.
- Round-level `continue` paths skip the tail `batch_bytes=0; ++round;`, while slot-level `continue` paths skip evaluation or commit for only the current slot. PART2 §5.2–5.4 records this control-flow sensitivity. Do not infer that all continues are equivalent or that moving declarations preserves their dynamic lifetime.
- On private repair success, `repair_proposals.at(slot)` receives moved dependencies/imported/patch/flags and scalar results. Later batch contact checks, final reservations, parameter changes, coverage assembly and publication read those proposals; final lane census commits use `lane_slots` by matching proposal index. Both arrays must stay index-aligned through the whole batch.

## Source limits

This is an inventory, not a behavior proposal. It does not recommend extracting or grouping state, changing collective placement, modifying early returns, or moving RAII objects. The production source and the Part2 inventory are fingerprinted in the companion manifest.
