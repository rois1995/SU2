# CUT INVENTORY, PART 2: the round loop of `Adapt` in `CNativeEngine3D.cpp`

Source read: a snapshot copy of `Common/src/adaptation/CNativeEngine3D.cpp` taken at the start of this
inventory (7444 lines, md5 `27237345887b00e3f5db0eba6e8795fb`, copy at
`/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-NativeIntegrated/f3d988e5-f14f-4a96-b6fb-963ec11730e7/scratchpad/cut/snap.cpp`).
All line numbers refer to that snapshot. Every block is also named by a quoted fragment, so it can be found if the live file shifts.

Fixed points:

| item | line | fragment |
|---|---|---|
| `Adapt` begins | 1237 | `EngineResult Adapt(World& world, const Owned& accepted,` |
| `result`/`stats` | 1315-1316 | `EngineResult result;` / `auto& stats=result.stats;` (stats is a reference INTO result) |
| round loop opens | 3285 | `for (size_t round=0;;) {` |
| slot loop opens | 3924 | `for (size_t slot=0;slot<(repairing?choices.size():slots);++slot) {` |
| slot loop closes | 5874 | `    }` after `if (!ready) return finish();` (5873) |
| round loop closes | 6115 | `  }` after `batch_bytes=0;++round;` (6114) |
| post-loop verdict | 6116-6131 | `Timer timer(stats.completion_seconds,...Certification)` |
| `Adapt` ends | 6132-6133 | `return finish();` `}` |

Brace depth was computed by a script (comments and string literals stripped). Loop body = depth 1, slot body = depth 2,
`evaluate_candidate` body = depth 3.

Counts used in the summary (loop 3285-6115 plus post-loop 6116-6132):
- `return finish()` occurrences: 108; `vote(` calls: 80.
- Round-level blocks: 41 (B01-B41) plus the post-loop verdict; slot-level blocks: 22 (S01-S22);
  sub-blocks of `evaluate_candidate`: 30 (E01-E30).
- Lambdas defined inside the loop: 36 named (`pick` ... `used`), 3 of them above 80 lines
  (`evaluate_candidate` 1476, `measure_private` 97, `place_patch` 96), 11 above 15 lines.

Notation. "Coll." = makes MPI collectives. Through `World`: `world.maximum`, `world.elect`, `world.exchange`, and
`vote`/`admit` (`vote` -> `elect_trial_failure` -> `world.elect` unless `private_repair`). Direct ones:
`TimedAllreduce`/`CPassiveComm::Allreduce`. These bypass the `World` counters `collective_calls`/`election_collective_calls`.
Every `Directory` method except `BeginConstructionPass`/`EndConstructionPass`/`PassDeferred`/`Changed*`/`ResidentBytes`
is collective: `CNativeDistributed3D.hpp`, "All ranks of world participate in matching calls". The same holds for
`CavityMetricSource::Import*`/`Sample*` without the `Local` suffix. Function-level lambdas `seed_next`, `enter_phase`,
`observe_progress`, `generation_census`, `begin_length_generation`, `phase_ownership`, `capture_checkpoint`,
`rollback_checkpoint`, `service_frontier`, `coalesce`, `measure`, `surface_ready`, `admit` and `finish` are all
collective (each votes, admits or exchanges). `update_fixed_metric_cells`, `record_rejection`, `geometry_trial`,
`metric_trial`, `height_check`, `surface_merit` and `length_direction` are local.

Units: Census, Queues, Operators, Trial, Commit, Exchange, Control, Ownership (as given by the caller).

---------------------------------------------------------------------------------------------------

## 1. THE ROUND AS AN ORDERED LIST OF BLOCKS

### 1.1 Round head: control decisions before any candidate work (3286-3624)

Every block in this part runs on every rank with replicated conditions. All mode variables (`phase`, `repairing`,
`coarsening`, `metric_cleanup`, `surface_target`, `bidirectional`, `standalone_coarsening`, `checkpoint.valid`) are
replicated. A `continue` at this level restarts the round WITHOUT `++round` and WITHOUT `batch_bytes=0`. Those two happen
only at B41.

| # | lines | name | purpose | runs when | ends round | Coll. | unit |
|---|---|---|---|---|---|---|---|
| B01 | 3286-3290 | PhaseScope | `std::optional<SU2NativeTiming3D::PhaseScope> phase_scope` labels the round Quality/Surface/Construction in the F3 ledger | `options.timings` | no | no | Control |
| B02 | 3291-3301 | SurfacePreparedExit | surface preparation done: check work and time budget, set `result.surface_prepared` | `surface_target && (result.surface_prepared \|\| (repairing && surface_ready()))` | always `return finish()` | yes: `surface_ready()` (only if repairing, short-circuit), `world.maximum` x1-2 | Control |
| B03 | 3302-3305 | LaneTimer | `timed_lane`, `untimed_lane`, `Timer lane_timer` (surface/interior seconds) for the bidirectional split lane | `bidirectional&&Construction&&!repairing&&!coarsening` gives lane 1/2, else a dummy | no | no | Control |
| B04 | 3306-3317 | RoundChoiceState + `pick` | declares `choice, complete, any, bad_field, refill`; defines `pick` | always | no | (pick: yes) | Queues |
| B05 | 3318-3340 | SelectionCensusAndPick | `++stats.selections`; `pending_census`; `dirty` via `world.maximum`; `bad_field`; `selection.EnableCollapseFilter()` when coarsening; `pick()`; `complete`; `refill` | always (inside `Timer ... Scheduling`) | no | yes: `world.maximum` x1-4, `vote` (coarsening), `pick` | Queues + Census |
| B06 | 3341 | ReadyGate | `if (!ready) return finish();` | always | maybe finish | no (finish: yes) | Control |
| B07 | 3342-3351 | SurfaceIdleSeed | an idle rank seeds the next census batch, re-picks, recomputes `complete`/`refill` | `surface_target && Construction && any && world.maximum(!choice.active && seed_slot<initial.size())` | maybe finish | yes: `seed_next`, `pick`, `world.maximum` x3-4 | Census |
| B08 | 3352-3355 | TimeBudget1 | elapsed-time stop | `options.maximum_seconds` | maybe finish | yes: `world.maximum` | Control |
| B09 | 3356-3363 | SurfaceRequiredHandoff | required surface debt interrupts a collapse pass: `begin_length_generation(false)`, `++stats.surface_required_handoffs`, `observe_progress("surface_required_entry")` | `surface_target && Construction && !repairing && coarsening && world.maximum(construction_invalid)` | `continue` / finish | yes | Control |
| B10 | 3364-3369 | CoarseningEntryRefill | before the first checkpoint, finish the entry census: `refill=true`, then `if(refill){ready=seed_next();...continue;}` | `standalone_coarsening && !checkpoint.valid && Construction && !repairing && max(cells!=working) && max(seed_slot<initial)`, OR `refill` from B05/B07 | `continue` / finish | yes | Census |
| B11 | 3370-3374 | CoarseningEntryToImprovement | a complete, valid entry census goes to Improvement | `standalone_coarsening && !checkpoint.valid && Construction && !repairing && !max(...)` | `continue` / finish | yes: `enter_phase` | Control |
| B12 | 3375-3377 | CleanQuality | `const bool clean_quality=!metric_cleanup && phase==Improvement && !world.maximum(...)` | always (the collective only runs if the first two terms are true) | no | yes (conditional) | Census |
| B13 | 3378-3385 | MetricCleanupExit | ends the short-edge cleanup pass: `++stats.length_collapse_passes`, `observe_progress("metric_cleanup_exit")`, `enter_phase(Improvement)` | `metric_cleanup && !any` | `continue` / finish | yes | Control |
| B14 | 3386-3393 | SizeRepairEntry | `admit`, `selection.EnableSizeRepairs(working)`, `observe_progress("size_repair_entry")` | `bidirectional && clean_quality && !selection.size_repairs && max(size_bad_cells)` | `continue` / finish | yes | Control (mode) + Queues |
| B15 | 3394-3402 | GeometryRepairEntry | `selection.EnableGeometryRepairs(working)`, `observe_progress("geometry_repair_entry")` | `bidirectional && clean_quality && !geometry_repairs && !max(size_bad) && max(construction_invalid)` | `continue` / finish | yes | Control + Queues |
| B16 | 3403-3410 | CoarseningRollback | a failed quality closure rolls back the checkpoint, or stops on the work budget | `standalone_coarsening && Improvement && checkpoint.valid && !any && !clean_quality && ... && max(checkpoint.trial)` | `continue` / finish | yes: `rollback_checkpoint` | Control |
| B17 | 3411-3428 | CoarseningCheckpointCycle | commit or rollback the trial checkpoint by global cell count (`TimedAllreduce SUM`), `discard_checkpoint`, a valid finish when there is no coarsening demand, `capture_checkpoint` + `enter_phase(Construction)` | `standalone_coarsening && clean_quality && !max(construction_invalid)` | `continue` / `return finish()` with `result.valid=true` | yes | Control (checkpoints) |
| B18 | 3429-3439 | QualityExitToConstruction | `selection.Progress`, `refine_geometry`, `observe_progress("quality_exit")`, `enter_phase(Construction,refine_geometry)` | long condition 3429-3431, short-circuited `world.maximum` terms | `continue` / finish | yes | Control |
| B19 | 3440-3448 | SurfaceLaneToInterior | `generation_census`, clear geometry priorities, `directory->EndConstructionPass()`, lane=Interior, `stats.split_lane=2` | `bidirectional && Construction && !repairing && !coarsening && !any && lane==Surface && !max(!scope_census_complete())` | `continue` / finish | yes | Control + Census |
| B20 | 3449-3457 | InteriorLaneToSurface | `generation_census`, `directory->BeginConstructionPass()`, lane=Surface, `split_lane=1` | same as B19 but lane==Interior and `max(surface_queue/geometry_queue nonempty)` | `continue` / finish | yes | Control + Census |
| B21 | 3458-3514 | GenerationEnd | ends a bidirectional generation: `productive`, `generation_census`, debt window (`fell` lambda), `length_blocks_dirty`, pass counters, `NATIVE3D_LENGTH` print, `observe_progress`; then a new split generation (`begin_length_generation(false)`), or `enter_phase(Improvement)`, or after collapse `length_direction(false)`+`EndConstructionPass`+`phase_ownership(false)`+`begin_length_generation(false)` | `bidirectional && Construction && !repairing && !any && !max(!scope_census_complete())` | always `continue` / finish | yes | Control + Census (debt) + Ownership (`phase_ownership`) |
| B22 | 3515-3530 | UnidirectionalNextGeneration | `selection.ChangePhase(false,...)` for a new finite generation | `!bidirectional && !coarsening && Construction && !repairing && !any` and census, demand and productive | `continue` / finish | yes | Control + Census |
| B23 | 3531-3534 | UnidirectionalConstructionDone | `enter_phase(Improvement)` | `!bidirectional && Construction && !repairing && (complete \|\| (!any && !max(...)))` | `continue` / finish | yes | Control |
| B24 | 3535-3541 | TimeBudget2 | elapsed-time stop (inside a Scheduling `Timer`) | `options.maximum_seconds` | maybe finish | yes | Control |
| B25 | 3542-3564 | MetricCleanupEntry | one short-edge cleanup pass: `admit`, `BeginConstructionPass`, `length_direction(true)`, `ChangePhase(false)`, sets `metric_cleanup=metric_cleanup_tried=true; repairing=false;` and the window counters | `bidirectional && !metric_cleanup && !metric_cleanup_tried && Improvement && !any && !clean_quality`, progress complete, `max(short_edges)` | `continue` / finish | yes | Control |
| B26 | 3565-3567 | ValidCompletionExit | `result.valid=true` | `!metric_cleanup && complete && Improvement && !max(gates)` | `return finish()` | yes | Control |
| B27 | 3568-3593 | StagnationWindow | every `options.stagnation_window` rounds: `++stats.progress_windows`, `mesh_progress`/`search_progress`, `observe_progress("window")`, `phase_ownership(true)` in Improvement, then stagnation stop or `enter_phase(Improvement)` | `stats.rounds>phase_start_round && round && round%stagnation_window==0` | `continue` / finish | yes | Control + Ownership |
| B28 | 3594-3599 | NoCandidateExit | `stats.stop=OperatorStagnation`; `vote(failure); return finish();` | `!any` | always finish | yes | Control |
| B29 | 3600-3601 | RoundLimit | `if(options.maximum_rounds && round>=options.maximum_rounds)break;` -> post-loop verdict | as written | `break` | no | Control |
| B30 | 3602-3618 | HistoryCapacity | grows `result.splits`/`result.collapses` capacity under `admit`; every rank votes | always | maybe finish | yes: `admit`, `vote` | Commit (storage) / Control (memory) |
| B31 | 3619-3624 | RepairEpoch | overflow guard, `++selection.repair_epoch` | always | maybe finish | yes: `vote` | Queues |

### 1.2 Round mode flags and round state (3625-3688)

| # | lines | name | purpose | runs when | ends round | Coll. | unit |
|---|---|---|---|---|---|---|---|
| B32 | 3625-3641 | RoundModeFlags + RoundState | `++stats.rounds`, lane round counters, `concurrent` (3627), `batched` (3630), `planned_coarsening` (3637); declares `choices, dependency_batch, imported_batch, reserved, slots=1`, the refused counters and `refused_receipts` | always | no | no | Control (flags) |
| B33 | 3642-3651 | `refuse` lambda | blocks the feature, counts the rejection, appends a receipt when `batched \|\| (!repairing && planned_coarsening)` | called from B34, B35e, S03a | no | no | Queues |
| B34 | 3653-3688 | `plan_choices` lambda | global work guard; for `planned_coarsening`: `directory->PlanBatch`, refusals, pass-deferred receipts, compaction of `choices`, `service_frontier`, no-progress guard | called from B35c, B36 | returns false -> caller finishes | yes | Queues + Exchange |

### 1.3 Candidate acquisition: batched or single (3689-3891)

| # | lines | name | purpose | runs when | ends round | Coll. | unit |
|---|---|---|---|---|---|---|---|
| B35 | 3689-3860 | BatchedAcquire | (whole batched path, 172 lines) | `batched` = `repairing \|\| (candidate_batch>1 && !(split_lanes&&!coarsening) && !curved && !composition && (!coarsening\|\|!bad_field))` | maybe finish | yes | see sub-blocks |
| B35a | 3690-3697 | BatchBytes | `batch_bytes=` the fixed per-batch bound; `admit(0)` | batched | maybe finish | yes | Control (memory) |
| B35b | 3699-3711 | RepairSlotShare | local debt sum from `repair_cell_severity`; `vote`; `complete` (SHADOWS round `complete`); `TimedAllreduce SUM`; proportional `repair_slots` | `repairing` | maybe finish | yes | Queues |
| B35c | 3712-3725 | BatchSelect | `selection.RepairBatch(working,repair_slots,oversized)` or `selection.Batch(working,candidate_batch)`; `vote`; `coalesce(choices)`; `plan_choices()`; `slots=TimedAllreduce(MAX)`; batch counters | batched | maybe finish | yes | Queues |
| B35d | 3726-3735 | BatchWorkAdmission | global proposal count (`TimedAllreduce SUM`) against `stats.work_limit`; `stats.global_work+=proposed` | batched | maybe finish | yes | Control (budget) |
| B35e | 3736-3768 | BatchLookupFetch | features per choice; `vote`; `directory->LookupBatch` unless planned; extra `batch_bytes` for oversized retained stars; `admit(0)`; `directory->FetchBatch`; refuse oversized stars (`refused_slots`); `vote` | batched | maybe finish | yes | Exchange |
| B35f | 3769-3832 | CurvedRepairSupportExpansion | for curved repairs, pick the worst geometry-invalid core cell per candidate, `LookupBatch` its 4 vertex stars, merge with version checks into `dependency_batch`, re-`FetchBatch` when any expanded | `repairing && curved` | maybe finish | yes: `admit`, `vote` x2, `world.maximum` x2, `LookupBatch`, `FetchBatch` | Exchange (+ Queues for the core choice) |
| B35g | 3833-3836 | RefusedReceiptService | `service_frontier({},false,false,&refused_receipts,count)` | `world.maximum(refused_receipt_count!=0)` | maybe finish | yes | Exchange (Queues receipts) |
| B35h | 3838-3860 | BatchReserve | `priorities` (repair only); `vote`; `directory->ReserveBatch`; winners; pass-deferred receipts (NOT in try, 3849 `throw std::logic_error`); `service_frontier`; `batch_reserved`/`conflicts`; no-progress exit | batched | maybe finish | yes | Exchange |
| B36 | 3861-3891 | SingleAcquire | inside a Scheduling `Timer`. For `(curved\|\|composition) && !repairing && !concurrent`: global metric priority election (composition only: 3 `TimedAllreduce MIN`), then owner election (`TimedAllreduce MIN`). For `repairing \|\| concurrent \|\| planned-coarsening-equivalent`: push `choice` into `choices`, `vote`, `coalesce`, `plan_choices`, `choice=choices.front()`. Then `stats.proposed`, work admission (`TimedAllreduce SUM`) | `!batched` | maybe finish | yes | Queues + Control (budget) |

Note: `repairing` implies `batched` (3630), so B36 never sees `repairing`. The `repairing ||` term at 3878 is dead.

### 1.4 Private-repair setup (3892-3923)

| # | lines | name | purpose | runs when | ends round | Coll. | unit |
|---|---|---|---|---|---|---|---|
| B37 | 3892-3921 | RepairProposalSetup | local `struct RepairProposal` (3892-3901); `repair_proposals`; `private_failure`, `private_work_used/reserved`, `private_deadline`. When repairing: retained bytes admission, `repair_proposals.resize`, `vote`, `eligible=TimedAllreduce SUM`, `private_optimizer_allowance` (function-level), `private_work_reserved`, `stats.global_work+=...` | declarations always; body if `repairing` | maybe finish | yes (repairing) | Control (budget) + Trial (container) |
| B38 | 3922-3923 | PrivateBaseline | `private_started` clock; `private_collectives = world.collective_calls+world.election_collective_calls` | always | no | no | Control |

### 1.5 The slot loop (3924-5874)

`for (size_t slot=0;slot<(repairing?choices.size():slots);++slot) {`. When repairing the bound is LOCAL (`choices.size()`).
Otherwise it is the global `slots` (MAX over ranks) in batched mode, or 1. NOTE THE INDENTATION: the slot body
(3925-5873) starts at the SAME 4-space indentation as the `for` line. The body of `evaluate_candidate`
(4109-5582) starts at the same indentation as its own declaration line 4108, and its last statement
`      return true;` (5582) is indented deeper than the body. Use brace matching, not indentation.

| # | lines | name | purpose | runs when | ends slot / round | Coll. | unit |
|---|---|---|---|---|---|---|---|
| S01 | 3925-3933 | SlotState + `reserve_dependencies` | `dependencies, imported, chosen, pass_deferred, collapse_corrected, collapse_apex, collapse_correction_refusal`; the lambda wraps `directory->Reserve` and records `pass_deferred`/`stats.length_expansion_deferrals` | always | no | (lambda: yes) | Exchange |
| S02 | 3934-3941 | BatchedSlotUnpack | `choice=choices[slot]`; move `dependency_batch.candidates[slot]` and `imported_batch.candidates[slot]`; `chosen=reserved.chosen[slot]`; skip | `batched` | `continue` (slot) if `repairing?!chosen:!world.maximum(chosen)` | yes when not repairing | Exchange (unpack) |
| S03 | 3942-4003 | SingleSlotAcquire | (non-batched path) | `!batched` | see sub-blocks | yes | Exchange |
| S03a | 3943-3971 | SingleLookupFetch | inside a Topology `Timer`. Planned coarsening: `if(!world.maximum(choice.active))continue;` then take `dependency_batch.candidates.at(0)`. Otherwise: `directory->LookupBatch`, `vote` lookup failure, oversized -> `refuse` + inactive, `vote`, `service_frontier` when any oversized, `if(!world.maximum(choice.active))continue;`. Then `directory->Fetch` | `!batched` | `continue` (slot) x2 / finish | yes | Exchange |
| S03b | 3972-3974 | AcquireValidity | missing dependencies or import gives `return finish()` | `!batched` | finish | no (finish: yes) | Control |
| S03c | 3975-3987 | CollapseCorrection | `directory->CorrectCollapse(...)`; work accounting; replaces `dependencies`/`imported`; sets `collapse_corrected`, `collapse_apex`, `collapse_correction_refusal` | `surface_target && curved && coarsening && !repairing` | maybe finish | yes | Operators (+ Exchange) |
| S03d | 3988-4001 | SingleReserve | `reserve_dependencies(...)`; conflicts and `selection.Note`; `admitted=world.maximum(chosen?2:pass_deferred?1:0)`; if 0 then `ready=false`; if 1 then `service_frontier(...)`, `stats.length_pass_deferred+=pass_deferred; continue;` | `!batched` | `continue` (slot) / falls to S03e | yes | Exchange |
| S03e | 4002 | SingleReadyGate | `if (!ready) return finish();` (no rank reserved and none deferred) | `!batched` | finish | - | Control |
| S04 | 4004-4025 | TrialSwitches | `trial_maximum`, `trial_reduce`, `trial_import_cells`, `trial_import_samples`, `trial_sample_points`, `trial_sample_midpoints`: local if `private_repair`, collective otherwise | always | no | switchable | Trial (Exchange adapter) |
| S05 | 4026-4049 | CandidateState | slot `failure` (shared by evaluation AND commit), `coordinate_only`, `private_scratch`, `old, fresh, point, approved, moved, repair_approved, cavity_inserted, optimization_approved, repair_optimization, certified, geometric_progress, last_geometry_operation, field`, IDENTITY RANGES `epoch, token, offset, construction_nodes, node_stride, node_offset` (4041-4046, from `round`, `world.rank`, `slot`), `capture, capture_selected, accepted_ladder, old_flags, fresh_flags, measured_flags` | always | no | no | Trial (state) + Commit (ids) |
| S06 | 4050-4064 | `note_ladder` | per-ladder trial counters (repair only) | called in E26, E27, E28 | no | no | Trial (counters) |
| S07 | 4065-4070 | TransactionObserver | `transaction_feature`, `observe_transaction` (calls `options.transaction_observer`) | always | no | no | Control (diagnostics) |
| S08 | 4071-4076 | CavityScope | `repair_budget, repair_work`; `source.SetCavity(&imported.cells)`; RAII `CavityScope` resets it at slot exit | always | no | no | Trial (Operators context) |
| S09 | 4077-4083 | PhysicalEdge | `physical_edge` = the chosen edge lies on a physical face | always | no | no | Trial |
| S10 | 4084-4107 | `boundary_replacement_ready` | mapped geometry: unchanged-face fast path, else `Parameters()->CheckReplacement`; catches std::exception, RETHROWS `bad_alloc` | used by E02 and S12 | no | no (assumed: CheckReplacement is documented as a complete local proposal) | Trial |
| S11 | 4108-5583 | `evaluate_candidate` | the whole candidate trial: operator, metric, joint placement, strip, growth, repair ladders (see 1.6) | called in S12 (private repair), S13 (private collapse or collective) | returns false -> caller fails | collective unless `private_repair` | Operators + Trial (+ Exchange inside joint growth) |
| S12 | 5584-5627 | PrivateRepairSlot | local deadline check (`break` slot loop); `private_repair=true`; `evaluate_candidate()`; `admit_coverage` lambda; `ExportParameters(old/fresh)`; `boundary_replacement_ready` veto; `private_repair=false`; `private_work_used+=repair_work`; MOVE slot state into `repair_proposals.at(slot)`; `continue` | `repairing` | `break` (slot loop) on failure or deadline / `continue` (slot) | no by design (private); not enforced, only counted in B40a | Trial + Commit (stores proposal) |
| S13 | 5628-5639 | CandidateEvaluation | private surface collapse: snapshot `world.election_collective_calls`, `world.collective_calls`, `stats.global_work`; `private_repair=true`; `evaluate_candidate()` in try; atlas time flag; `private_repair=false`; fail if any counter moved; `vote`. Otherwise the collective `else if(!evaluate_candidate())return finish();` | `surface_target && coarsening && !repairing && !observe_rejections` / else | finish on failure | yes (vote) / yes | Trial |
| S14 | 5640-5643 | RejectionCarry | append " Bounded correction: ..." to `result.reason`; save `pending_rejection` | always | no | no | Trial (diagnostic) |
| S15 | 5644-5654 | SingleClearance | `surface_edit=approved && !SameSkin(old,fresh)`; interior-boundary discovery counter; `directory->Clearance(...)`; deferred -> `chosen=false` + counters; veto -> `geometry_trial(false)` | `(curved && max(surface_edit)) \|\| max(coordinate_only && approved) \|\| max(collapse_corrected && approved)` (short-circuit) | finish on protocol failure | yes | Exchange |
| S16 | 5655-5682 | FinalReservation | completeness/version check of the final old footprint; `vote`; `++stats.final_reservations`; `reserve_dependencies(...)`; conflict -> deferred counters; `vote`; `commits=TimedAllreduce SUM`; `maximum_parallel_commits` | `(concurrent \|\| max(collapse_corrected)) && max(approved)` | finish | yes | Exchange |
| S17 | 5683-5692 | ParameterEditsAndPoint | restore `pending_rejection`; `ExportParameters` + `BoundaryParameterState::Changes` -> `parameter_edits` (an exception sets `approved=false`); resolve the split `point` from `fresh`; `observe_transaction("transaction_end")`; `vote` if an observer is set | always | finish | yes (observer only) | Commit (parameters) |
| S18 | 5693-5701 | RejectionRecord | `last_metric_rejection`, `last_rejected_feature`, centroid, `last_rejection`, `record_rejection` | `chosen && !approved` | no | no | Trial (diagnostic) / Control |
| S19 | 5702-5767 | Routing | inside a routing `Timer`: `++stats.rejected`; build `removals/additions/blocked` per owner (parents layout, `PublishMetrics`, physical faces, coordinate-only version bump); `vote`; `world.exchange` x3 under a common baseline | always | finish (5767) | yes | Commit (rows, metric transport) + Exchange (routing) |
| S20 | 5768-5773 | Publish | `directory->Publish(working,removed,added,...,parameter_edits, approved?fresh.coverage:no_coverage, ..., repairing)` | always | finish | yes | Exchange (publication) |
| S21 | 5774-5784 | PostPublishReceipts | `update_fixed_metric_cells`; split commit stats (`TimedAllreduce SUM`, bidirectional construction); `progress_consistent=false`; `length_pass_deferred`; `disposition`; `service_frontier(...)` | always | finish | yes | Commit + Exchange (receipts) |
| S22 | 5785-5873 | SelectionUpdate | admission (`admit`); checkpoint changed set; retire edges + `selection.Drop`; `progress_consistent`; `changed`; `selection.Block(failures)`; accepted counters, `result.splits/collapses.push_back`; child/parent ratio; `vote`; `measure(changed,...)` (or the dead `batched&&repairing` path); `FinishRepairs`, `TouchRepairSupport`, `Touch`, `ServiceClaimedVertex`; `vote`; `retired_edges.clear()` | always | finish (5873) | yes | Census (measure) + Queues (Block/Touch) + Commit (counters, history) |

### 1.6 Inside `evaluate_candidate` (4108-5583), in order

| # | lines | name | purpose | runs for | Coll. (non-private) | unit |
|---|---|---|---|---|---|---|
| E01 | 4109-4118 | CoordinateAdmissionAndCapture | `coordinate_stream=trial_maximum(coordinate_only)`; `admit(private_scratch)`; selects the capture feature (`transaction_captured` is function-level) | all | yes | Trial |
| E02 | 4119-4125 | `guarded_geometry` | first heights (surface), `height_check`, `boundary_replacement_ready`, `geometry_trial`, observation | all operator results | no | Trial |
| E03 | 4126-4132 | `repair_kind` | Repair vs GeometryRepair (`HeightScore` before/after) | repair | no | Trial |
| E04 | 4133-4172 | `geometric_change` | `GeometryGuidanceExcess` on old/fresh; coarsen and repair rules; rejection via `guarded_geometry(false)` | curved | no | Trial |
| E05 | 4173-4174 | ObserveBegin | `observe_transaction("begin")`; `vote` if an observer is set | all | conditional | Control (diag) |
| E06 | 4175-4186 | `validate_metric_vector` | metric validation (non-surface: `ValidateMetricChange` on cells; surface: `MeasureSurfacePlanning` + merit) | only the dead E13 path | no | Trial |
| E07 | 4187-4216 | CreaseClassification | `splitPosition, centralPosition, split_failed, central_available`; interior-boundary counter; crease freedom for undeclared folded facet pairs; `incompatible_crease` lambda (4209-4215) sets the slot `failure` | composition split on a long physical edge | no | Trial (pre-operator) |
| E08 | 4217-4246 | CompositionSplitPosition | `vote`; `admit`; seed donor import (`trial_import_cells`); `FrozenField`; midpoint-only crease conflict; `centralPosition`, `BalancedSplitPoint`; `vote` | `!repairing && !coarsening && composition` | yes | Operators (placement) + Exchange (donors) |
| E09 | 4247-4290 | PrimaryOperator | in a Candidate `Timer`: `++stats.attempted`; identity-range overflow guard; `old=GeometryPatch(imported,&geometry)`. Collapse: `CollapseOutsideTolerance` / `ReconnectCollapse` / `CollapseVertex`. Split: `SplitEdge`, then `InsertEdgeCavity`, then a retry at `centralPosition`. Then `ready=vote(failure)` | `chosen && !split_failed`; operator only for `!repairing` | yes (vote) | Operators |
| E10 | 4291-4297 | StreamSetup | `metric_needed`; `sensors.valid=true`; `streamed=ready` (ALWAYS true here); `admit(private_scratch)`; `++stats.private_query_retries` | all | yes | Trial |
| E11 | 4298-4313 | OptimizerBudget | `reserve_optimization`, `release_optimization` lambdas; `reserve_optimization(!streamed && ...)` = `reserve_optimization(false)`, still 2 `trial_reduce` in collective mode; `trial_maximum(repair_budget>repair_work)` | all | yes | Control (budget) |
| E12 | 4314-4342 | `optimize_patch` | `OptimizedRepairPatch` + 20 counter lines | repair (E26, E28) | no | Operators |
| E13 | 4343-4428 | NonStreamedMetricAndTryRepair | DEAD: `if (!streamed && metric_needed)` never holds because `streamed==true`. Contains `try_repair` (4360-4420). Only the Metric `Timer` and the subtraction lines 4426-4427 run | none (dead) | no | Trial/Operators |
| E14 | 4429-4435 | ReleaseAndDeadRetry | `release_optimization()` (1 `trial_reduce`); 4430-4435 dead (`!streamed`) | all | yes | Control |
| E15 | 4436-4550 | StreamedOldMeasurement | opens `if (streamed) {` (always true; this block runs to 5579). `measure_private` lambda (4438-4534); `reuse` from `selection.Cached` + `vote`; `before` = old measurement (`measure_private(metric_needed?old.cells:empty,...)`); old flags; `vote(old_failure)` | all | yes | Trial (+ Exchange donors) |
| E16 | 4551-4617 | `reserved_relocations` + `validate_private` | identity count of relocated apices; full private validation (see section 3) | all | yes (conditional vote) | Trial |
| E17 | 4618-4620 | PrimaryValidation | `approved=validate_private(metric_needed && old_measured && approved, GeometryRefine or construction_kind)` | all | yes | Trial |
| E18 | 4621-4646 | JointClassification | `boundary_edge`, `joint`, `flat_edge`, `strip`, `wallApices`, `wallSplit`, `placement_obstruction`; `place_patch` lambda starts at 4647 | all (`joint` only for a failed composition/geometric split) | no | Trial (route) |
| E19 | 4647-4742 | `place_patch` | joint placement transaction (96 lines; see section 3) | E20, E22 | yes | Operators + Exchange + Trial |
| E20 | 4743-4757 | HeightApexPlacement | `height_apex` choice (wall apex with 2 wall faces); `place_patch(height_apex!=MAX,{height_apex},height_apex)`; `place_patch(joint && composition && ... ,{})` | all (collectives always: 2 `trial_maximum` at least) | yes | Operators |
| E21 | 4758-5035 | StripSearch | `initial_placement_obstruction`; `movers`; when `trial_maximum(strip\|\|wallSplit)`: Lookup/Fetch/version check/reserve/re-measure/donors; strip move seeds (metric mean, damped stiffness); normal moves; extra split edge; frames (Reconnect fan); up to `36*frames+12` trials of `SplitSurfaceStrip` (+ `InsertEdgeCavity`/`SplitEdge` on an extra edge) with work/time guards and `validate_private` | joint composition flat strips and wall splits | yes | Operators + Exchange + Trial |
| E22 | 5036-5118 | ApexSearch | `movable_star` lambda; `search`; `apices` from `LookupBatch`+`FetchBatch` (cost classification, `SplitPatchApices`, `admitted_cells` lambda, targeted obstruction); up to 8 `place_patch` attempts, each with optional obstruction expansion (`LookupBatch`+`FetchBatch`) | joint geometric/flat composition, no wall apices | yes | Operators + Exchange |
| E23 | 5119-5140 | BoundaryPlacementTensors | `joint=joint && boundary_edge`; `trial_sample_points`; `placement`/`nearPlacement` via `composed_sample` | joint boundary | yes | Trial (metric query) |
| E24 | 5141-5360 | GrowthLoop | up to 12 growths: work/time guards; frames (`RefitWallSplit`, `ReconstructWallSplit`, `ReconstructSplitPatch`); `validate_joint` lambda; `missing_closure`; a 2-strategy `RepairMetricConnectivity` with node renaming; obstruction selection; grow: `LookupBatch` (unconditional, empty for non-growers), version-checked merge, `Fetch`, `reserve_dependencies`, `observe_transaction("growth_reserved")`, re-measure | joint boundary | yes | Operators + Exchange + Trial |
| E25 | 5361-5409 | FallbackSetup | `fallback=metric_needed && old_measured && repairing && !size_repairs`; dead `if (fallback && !repairing)`; `move`, `vertex`, `nodes` (Candidate `Timer` + `vote`); metric move centre (`trial_sample_midpoints` batches of `MaximumCavityCells`, `composed_sample`, `MetricMeanPoint`) | `nodes` all; centre for composition repair | yes (local when private) | Trial + Operators |
| E26 | 5410-5439 | `optimize_repair` | bounded coupled optimizer: `reserve_optimization(true-ish)`, donor import, `FrozenField`, `optimize_patch`, `release_optimization`, `validate_private(...,Repair)`, `note_ladder(1,...)` | repair (not surface, not geometry_repairs) | local (private) | Operators + Trial |
| E27 | 5440-5491 | FallbackLadder | 19 trials: 0-2 `MoveVertex` (steps 1, .5, .25 toward the neighbour or metric centre); trial 3 first calls `optimize_repair`; 3.. `CollapseVertex` (vertex repair) or `Reconnect`; `validate_private(...,Repair)`; `note_ladder` | repair (`fallback`) | local (private) | Operators + Trial |
| E28 | 5492-5576 | GeometryRepairSplits | `reserve_optimization(true)`; `geometry_counts` lambda (5497-5520, uses `atlas_budget.CheckTime()`); boundary-failed edges; donors; per edge: `SplitSurfaceStrip`, geometry counts, `validate_private(GeometryRepair)`, optional `optimize_patch` reclose; LOCAL wall-clock `break` at 5539; `release_optimization` | `geometry_repairs && !coordinate_only && chosen && !approved && curved` | local (private) | Operators + Trial |
| E29 | 5577-5578 | SizeRepairOptimize | `if(selection.size_repairs && !optimize_repair())return false;`; dead `fallback_accepted` | size repairs | local | Operators |
| E30 | 5580-5582 | ObserveEnd | `observe_transaction("construction_end")`; `vote` if an observer is set; `return true` | all | conditional | Control (diag) |

### 1.7 Repair batch commit after the slot loop (5875-6112) and round end

| # | lines | name | purpose | runs when | ends round | Coll. | unit |
|---|---|---|---|---|---|---|---|
| B40 | 5875-6112 | RepairBatchCommit | (whole private-repair commit, 238 lines) | `repairing` | see below | yes | see sub-blocks |
| B40a | 5876-5884 | PrivateWorkStats | `busy_seconds`, `private_work_used`, `NATIVE3D_PRIVATE_WORK` print (Output scope), `private_tracked_collectives += world counters - private_collectives` | repairing | no | no | Control |
| B40b | 5885-5899 | PrivateJoin | IIFE `used` = `CPassiveComm::Allreduce SUM` under a JoinWait scope; `join_wait_seconds`; over-use -> failure, else refund `stats.global_work`; `private_deadline` + `world.maximum` -> TimeBudget; `vote(private_failure)` | repairing | finish | yes | Control (budget, deadline) |
| B40c | 5900-5926 | ContactCandidates | per proposal: completeness/version check (copy of S16), `needs_contact`; `contact.push_back({&old,&fresh,approved,false,priority,coordinate_star,{}})` (Clearance scope); `vote` | repairing | finish | yes | Exchange |
| B40d | 5927-5939 | ClearanceBatch | `directory->ClearanceBatch(working,contact,...)`; deferred -> `chosen=false` + counters; veto -> `geometry_trial(false)`, ladder `rejected[5]`; `approved=checked.candidate`; reason | `world.maximum(needs_contact)` | finish | yes | Exchange |
| B40e | 5940-5955 | FinalReserveBatch | `final_inputs` (MOVES `proposal.imported` of approved proposals), priorities, `vote`; `++stats.final_reservations`; `directory->ReserveBatch` | repairing | finish | yes | Exchange |
| B40f | 5956-5969 | CommitCount | moves imports back; losers -> ladder `rejected[6]`, conflicts; `commits` (`TimedAllreduce SUM`); `maximum_parallel_commits` | repairing | no | yes | Exchange |
| B40g | 5970-6036 | BatchRouting | removals/additions/blocked per owner; parameter edits; merged `coverage` (duplicate face/trace throws); `PublishMetrics`; physical faces; coordinate version bump; rejection bookkeeping for chosen-unapproved; `receipts`; observer `transaction_end` per captured proposal; `vote` | repairing | finish | yes | Commit + Exchange |
| B40h | 6037-6048 | BatchExchange | `world.exchange` x3 with an accumulating baseline; a distinct reason per stage | repairing | finish | yes | Exchange |
| B40i | 6049-6053 | BatchBytes | `batch_bytes += ...`; `admit(0)` | repairing | finish | yes | Control |
| B40j | 6054-6058 | BatchPublish | `directory->Publish(...,parameter_edits,coverage,...,true)` | repairing | finish | yes | Exchange |
| B40k | 6059-6061 | BatchReceipts | `update_fixed_metric_cells`; `progress_consistent=false`; `service_frontier({},false,false,&receipts,receipt_count)` | repairing | finish | yes | Commit + Exchange |
| B40l | 6062-6112 | BatchSelectionUpdate | `admit`; drop + retire edges + checkpoint changed; `batch_changed`; `Block(failures,true)`; per-approved counters (quality commits, `AccountQualityChurn`, ladder published/debt, optimization, progress, inserted/moved/reconnected); `vote` + `measure(batch_changed,...)`; `FinishRepairs`, `TouchRepairSupport`, `Touch`; `progress_consistent=true`; `vote`; `batch_changed.clear(); retired_edges.clear();` | repairing | finish | yes | Census + Queues + Commit |
| B41 | 6114 | RoundAdvance | `batch_bytes=0;++round;` | every round that reaches the loop end | no | no | Control |
| Post | 6116-6132 | CompletionVerdict | `incomplete` code (`world.maximum`); `result.valid` (`world.maximum`); `stats.stop`; reason text; `return finish();` | after B29 `break` | returns | yes | Control |

---------------------------------------------------------------------------------------------------

## 2. LOCAL STATE OF THE ROUND

### 2.1 Round-level variables (depth 1)

| name | type | line | purpose | written by | read by |
|---|---|---|---|---|---|
| `phase_scope` | `optional<PhaseScope>` | 3286 | F3 phase label for the whole round | B01 | (RAII) |
| `timed_lane` | `unsigned` | 3302 | 0/1 surface/2 interior lane of this round | B03 | lane_timer, B32 (3626), S03d (3993-3994), E07 (4188), S15 (5645) |
| `untimed_lane` | `double` | 3303 | sink for the lane timer when untimed | lane_timer | - |
| `lane_timer` | `Timer` | 3304 | surface/interior seconds | B03 | (RAII; destructs after `finish()` returns) |
| `choice` | `Choice` | 3306 | the current candidate; whole-round in single mode, per slot in batched mode | `pick` (B04/B05/B07), B36 (3869-3876, 3882), S02 (3935), S03a (3962 `choice.active=false`) | B07, B36, S02-S22, all of E |
| `complete` | `bool` | 3307 | selection says nothing is dirty | B05, B07 | B23, B26 (shadowed at 3705, 3806, 3826) |
| `any` | `bool` | 3307 | some rank has an active choice | `pick` | B07, B13, B16, B18-B23, B25, B28 |
| `bad_field` | `bool` | 3307 | selection invalid (dirty==2) | B05, B07 | `pick`, B32 (`batched`, 3631), E10 (`metric_needed`, 4292) |
| `refill` | `bool` | 3307 | census refill needed | B05, B07, B10 | B10 |
| `pick` | lambda | 3308 | selection of `choice` | - | B05, B07 |
| `clean_quality` | `const bool` | 3376 | Improvement with closed quality gates | B12 | B14-B18, B25 |
| `concurrent` | `const bool` | 3627 | several proposals per rank in flight | B32 | B36, `validate_private` (4561), `place_patch` (4690, 4707), E21 (4812, 4962), E24 (5142, 5349), S16 (5655) |
| `batched` | `const bool` | 3630 | batched acquisition mode | B32 | `refuse`, B35/B36 branch, S02/S03 branch, S22 (5789, 5814, 5850, 5858, 5871) |
| `choices` | `vector<Choice>` | 3632 | this round's batch | B35c, `plan_choices` (compaction), `coalesce`, B36 (3880) | `refuse` (via index), B35e-h, B36 (3882), B37, S02, slot bound when repairing |
| `dependency_batch` | `DependencyBatch` | 3633 | complete-star metadata per slot | `plan_choices` (PlanBatch), B35e (LookupBatch), B35f (merge, `original.cells`) | B35e-f, S02 (move per slot), S03a (`.at(0)` move) |
| `imported_batch` | `ImportedBatch` | 3634 | fetched payload per slot | B35e, B35f (re-fetch) | B35e, B35h (`ReserveBatch`), B37 (coordinate star, sizes), S02 (move) |
| `reserved` | `ReservationBatch` | 3635 | batch reservation result | B35h | B37 (`chosen`), S02 (`chosen[slot]`) (shadowed at 4316, 4684, 4806, 5344) |
| `slots` | `size_t` | 3636 | global slot count (MAX) | B35c | slot loop bound (non-repair) |
| `planned_coarsening` | `const bool` | 3637 | complete-star planning for parallel collapse | B32 | `refuse`, `plan_choices`, B35e (3743), S03a (3947) |
| `refused_candidates`, `refused_slots`, `pass_deferred_candidates` | `size_t` | 3640 | refusal/deferral tallies | `refuse`, `plan_choices`, B35e, B35h | B35c (3724), B35d, B35h, `plan_choices` |
| `refused_receipts`, `refused_receipt_count` | `array<RepairReceipt,MaximumCandidateBatch>`, `size_t` | 3641 | frontier receipts of refused/deferred proposals | `refuse`, `plan_choices`, B35h | `plan_choices`, B35g, B35h (`service_frontier`) |
| `refuse`, `plan_choices` | lambdas | 3642, 3653 | see section 3 | - | B35, B36, S03a |
| `RepairProposal` | local struct type | 3892 | the private result of one repair slot | - | S12, B40 |
| `repair_proposals` | `vector<RepairProposal>` | 3902 | one per choice (repair) | B37 (resize), S12 (fill, moves), B40d-f (approved/chosen/imported) | B40c-l |
| `private_failure` | `CLocalFailure` | 3903 | the private-path failure, voted once | B37, S12, B40b | B40b |
| `private_work_used`, `private_work_reserved` | `uint64_t` | 3903 | the private optimizer work ledger | B37 (reserved), S12 (used) | B40a, B40b |
| `private_deadline` | `bool` | 3904 | a local deadline was hit | S12 (5586), B40b | B40b |
| `private_started` | `Clock::time_point` | 3922 | busy-time start | B38 | B40a |
| `private_collectives` | `uint64` | 3923 | `World` collective baseline | B38 | B40a |

Function-level state written by the loop that crosses blocks (owned by the part-1 helper, listed for coupling):
`ready` (written by pick, B05, B10, B30, B35*, S03*, every `ready=vote(...)`, `measure_private`, S19, S20, S22),
`result.reason/valid/surface_prepared/splits/collapses`, `stats.*` (via `stats` = `result.stats`), `selection`,
`working` (only through `Publish`), `batch_bytes`, `batch_changed`, `batch_touched`, `batch_supports`, `retired_edges`,
`checkpoint`, `improvement_changed`, `construction_changed`, `construction_generation_changed`, `metric_cleanup`,
`metric_cleanup_tried`, `repairing` (only B25 inside the loop), `coarsening`/`construction_kind` (via `length_direction`
in B21/B25), `phase_start_round`, `progress_at_window`, `attempted_at_window`, `deferred_at_window`, `generation_window*`,
`length_blocks_dirty`, `construction_claims`, `transaction_captured`, `last_rejection`, `private_repair`,
`private_optimizer_allowance`, `atlas_budget.time_exhausted`. Read-only ID bases: `maximum_cell`, `maximum_node`
(fixed before the loop, 2169-2376).

### 2.2 Slot-level variables (depth 2) that cross blocks

| name | type | line | purpose | written by | read by |
|---|---|---|---|---|---|
| `dependencies` | `Dependencies` | 3925 | the complete versioned star of the candidate | S02, S03a, S03c, `place_patch` (4680), E21 (4802), E24 (5338) | S03a/c, `observe_transaction`, `place_patch`, E21, E24, S12 (moved), S16, S19 |
| `imported` | `Imported` | 3926 | payload; `source` cavity points at `imported.cells` | S02, S03a, S03c, `place_patch`, E21, E24 | everything; S12 (moved) |
| `chosen` | `bool` | 3927 | this rank holds the reservation | S02, S03d, S15 (deferred), S16 (deferred) | everywhere in E, S12-S22 |
| `pass_deferred` | `bool` | 3927 | the directory deferred to the next pass | `reserve_dependencies` | S03d, S19 (5749), S21 (5782-5784) |
| `collapse_corrected`, `collapse_apex`, `collapse_correction_refusal` | `bool, Id, string` | 3927-3928 | bounded collapse correction | S03c | E09, S13 (5640), S15, S16, S19 (parents) |
| `failure` | `CLocalFailure` | 4026 | the SHARED failure accumulator of evaluation and commit | E07-E30 (many), S17, S19, S22 | every `vote(failure)` in S11 and S17-S22 |
| `coordinate_only` | `const bool` | 4027 | repair moves a coordinate star larger than the cavity | S05 | E01, E25-E28, `validate_private`, S12, S15, S19 |
| `private_scratch` | `const size_t` | 4028 | the admission size of private work | S05 | E01, E10, E11, `boundary_replacement_ready`, `place_patch`, E21-E28, S12, S15, S16 |
| `old`, `fresh` | `Patch` | 4034 | the old cavity and its replacement | E09, `place_patch`, E21, E24, E25-E28, S12 (ExportParameters), S17 | E (all), S12 (moved), S14-S22 |
| `point` | `Node` | 4035 | the split point / moved vertex | E09, (E13 dead), E21, E27, E28, S17 (5690) | `observe_transaction`, `place_patch`, E24, S12, S22 (`result.splits`, child ratio) |
| `approved` | `bool` | 4036 | the candidate is certified | E09, E17, `place_patch` (4690, 4737), E21, E24, E26-E28, S12, S15, S16, S17 | E (guards), S12-S22 |
| `moved`, `repair_approved`, `cavity_inserted`, `optimization_approved` | `bool` | 4036-4037 | the kind of accepted edit | E09, E17, E21, E24, E26-E28, S12 | S12, S19 (parents), S22 (counters) |
| `repair_optimization` | `OptimizationOperations` | 4037 | optimizer counters | `optimize_patch` | S12, S22 |
| `certified` | `MetricDiagnosis` | 4038 | the last metric certificate | `validate_private` (reset + restore), `validate_metric_vector`, `measure_private` (shape), `place_patch` (4738) | E21-E28 decisions (`certified.failure`), `note_ladder`, S12, S18, S21 (`disposition`), S22 |
| `geometric_progress` | `bool` | 4039 | the frozen geometry error dropped | `geometric_change`, S12 | `validate_private`, E28, S12, S22 |
| `last_geometry_operation` | `const char*` | 4039 | operator name for diagnostics | `guarded_geometry` | `validate_private` (4594, 4602) |
| `field` | `unique_ptr<FrozenField>` | 4040 | the frozen field of the dead E13 path | E13 (dead), E14 (`reset`, dead) | E13 (dead) |
| `epoch`, `token`, `offset`, `construction_nodes`, `node_stride`, `node_offset` | `const` ids | 4041-4046 | GLOBAL ID RANGES for new cells/nodes, from `round`, `world.rank`, `slot` | S05 | E09, E21, E24, E27, E28, `optimize_patch`, `reserved_relocations`, `observe_transaction`, S12 |
| `capture`, `capture_selected` | `bool` | 4047 | the transaction capture switch | E01 | `observe_transaction`, E24 (5355), S12 |
| `accepted_ladder` | `unsigned` | 4048 | the ladder index of the accepted repair | `note_ladder` | S12 |
| `old_flags`, `fresh_flags`, `measured_flags` | `vector<CellQualityFlag>` | 4049 | per-cell q/J flags for churn accounting | `measure_private`, E15, `validate_private` | S12 |
| `transaction_feature` | `const Feature` | 4065 | the observer key | S07 | E01, `observe_transaction`, `validate_private` |
| `repair_budget`, `repair_work` | `size_t` | 4071 | optimizer work in this slot | `reserve_optimization`, `release_optimization`, `optimize_patch`, E28 | E26-E28, S12 (`private_work_used`, proposal) |
| `cavity_scope` | RAII | 4076 | resets `source.SetCavity(nullptr)` at slot exit | S08 | - |
| `physical_edge` | `const bool` | 4077 | the edge lies on a physical face | S09 | E07, E08, E09, S15 (5645) |
| `pending_rejection` | `const string` | 5643 | keeps this rank's refusal through the peer votes | S14 | S17 (5683) |
| `surface_edit` | `const bool` | 5644 | the skin changed | S15 | S15 |
| `parameter_edits` | `vector<BoundaryParameterEdit>` | 5684 | mapped parameter deltas | S17 | S20 |
| `removals`, `additions`, `blocked` | `vector<vector<...>>(world.size)` | 5702-5704 | outgoing routing | S19 | S19 exchange |
| `removed`, `added`, `failures` | vectors | 5705-5707 | incoming routed rows | S19 | S20, S21, S22 |
| `changed` | `vector<Id>` | 5785 | added cell ids to measure | S22 | S22 |

### 2.3 `evaluate_candidate`-level variables (depth 3) that cross sub-blocks

`coordinate_stream` (4109), `splitPosition`/`centralPosition`/`split_failed`/`central_available` (4187: E07->E08->E09),
`crease_freedom`/`crease_midpoint` (4189: E07->E08), `diagnostic_limit` (4190), `metric_needed` (4292: E10->E15, E17, E25),
`sensors` (4293), `streamed` (4294, always true), `before` (4542: E15; rewritten by `place_patch` 4696, E21 4816,
E24 5356; read by `validate_private` 4568/4578, E21 4901), `reuse` (4535: E15; cleared and rebuilt by `place_patch`
4680-4681, E21 4802-4803, E24 5338-5339), `empty` (4543), `old_measured` (4544: ->E17, E25), `boundary_edge`, `joint`,
`flat_edge`, `strip`, `wallApices`, `wallSplit`, `placement_obstruction` (4621-4646: E18->E20/E21/E22/E24; `joint` and
`approved` are mutated by `place_patch`), `height_apex` (4743), `initial_placement_obstruction` (4758), `movers` (4760),
`search`/`apices` (5043-5044: E22), `placement`/`nearPlacement` (5120: E23->E24), `fallback`/`move`/`vertex`/`nodes`/
`moveCenter` (5361-5372: E25->E27).

---------------------------------------------------------------------------------------------------

## 3. LAMBDAS DEFINED INSIDE THE LOOP

"Coll." = makes MPI collectives when `private_repair==false`. Lambdas marked "switch" go through
`trial_*`/`vote`/`admit` and become local when `private_repair==true`. Directory calls never become local.

| lambda | lines | len | purpose | state written | state read (short) | lambdas called | Coll. | unit |
|---|---|---|---|---|---|---|---|---|
| `pick` | 3308-3317 | 10 | `selection.Best`, `FilterCollapse` loop, global `any` | `choice`, `ready`, `any` | `coarsening, repairing, bad_field, bidirectional, working` | `vote` | yes | Queues |
| `fell` | 3471 | 1 | relative decrease test (no capture) | - | - | - | no | Control |
| `refuse` | 3642-3651 | 10 | refuse an oversized star | `selection` (Block), stats, `refused_receipts/count`, `refused_candidates`, `last_rejection` | `batched, repairing, planned_coarsening` | `record_rejection` | no (may throw logic_error) | Queues |
| `plan_choices` | 3653-3688 | 36 | planned complete-star selection for collapse | `choices`, `dependency_batch`, `refused_*`, `pass_deferred_candidates`, `ready`, `result.reason`, stats | `planned_coarsening, stats` | `vote`, `refuse`, `service_frontier`, `bytes` | yes | Queues + Exchange |
| `reserve_dependencies` | 3929-3933 | 5 | `directory->Reserve` + pass-deferred bookkeeping | `pass_deferred`, `stats.length_expansion_deferrals` | `chosen` | - | yes | Exchange |
| `trial_maximum` | 4005 | 1 | `world.maximum` or identity | - | `private_repair` | - | switch | Trial/Exchange |
| `trial_reduce` | 4006-4008 | 3 | `TimedAllreduce` or identity (generic `auto`) | - | `private_repair` | - | switch | Trial/Exchange |
| `trial_import_cells` | 4009-4011 | 3 | `source.ImportCells[Local]` | source stats | `private_repair, imported.cells` | - | switch | Exchange |
| `trial_import_samples` | 4012-4016 | 5 | `source.ImportMetricSamples[Local]` | source stats | same | - | switch | Exchange |
| `trial_sample_points` | 4017-4020 | 4 | `source.SamplePoints[Local]` | source stats | same | - | switch | Exchange |
| `trial_sample_midpoints` | 4021-4025 | 5 | `source.SampleMidpoints[Local]` | source stats | same | - | switch | Exchange |
| `note_ladder` | 4050-4064 | 15 | repair ladder counters | `stats.quality.ladder`, `accepted_ladder` | `repairing, certified, stats.*_rejected` | - | no | Trial (counters) |
| `observe_transaction` | 4066-4070 | 5 | user observer callback | (callback) | `capture`, all slot state | - | no | Control (diag) |
| `boundary_replacement_ready` | 4084-4107 | 24 | mapped-boundary replacement check | `fresh.parameters` (ExportParameters), `result.reason` | `phase, old, fresh, geometry` | `bytes` | no (assumed) | Trial |
| `evaluate_candidate` | 4108-5583 | 1476 | the whole candidate trial | almost all slot state, stats, `ready`, `result.reason`, `transaction_captured`, `last_rejection`, `selection` (Cached reads only) | everything | all of E | switch + directory | Operators + Trial + Exchange |
| `guarded_geometry` | 4119-4125 | 7 | first-height, height, boundary and geometry trial wrapper | `last_geometry_operation`, stats | `old, fresh, selection.geometry_repairs` | `height_check`, `boundary_replacement_ready`, `geometry_trial`, `observe_transaction` | no | Trial |
| `repair_kind` | 4126-4132 | 7 | MetricChange kind of a repair | - | `surface_target, selection, options.wall_heights, old, fresh` | - | no | Trial |
| `geometric_change` | 4133-4172 | 40 | frozen geometry-guidance gate | `geometric_progress`, stats, `result.reason` | `curved, surface_target, phase, old, fresh, selection` | `guarded_geometry` | no | Trial |
| `validate_metric_vector` | 4175-4186 | 12 | metric validation (only the dead E13 path) | `certified`, `result.reason` | `old, fresh, surface_target` | `geometric_change`, `metric_trial`, `surface_merit` | no | Trial |
| `incompatible_crease` | 4209-4215 | 7 | crease conflict failure | slot `failure`, stats | `choice, imported` | - | no | Trial |
| `reserve_optimization` | 4298-4305 | 8 | optimizer work reservation | `repair_budget`, `stats.global_work` | `private_repair, private_optimizer_allowance, repair_work` | `trial_reduce` | switch | Control (budget) |
| `release_optimization` | 4306-4310 | 5 | refund unused optimizer work | `repair_budget`, `stats.global_work` | same | `trial_reduce` | switch | Control (budget) |
| `optimize_patch` | 4314-4342 | 29 | `OptimizedRepairPatch` + about 40 counters | `optimization_approved`, `repair_optimization`, `repair_work`, `fresh`, stats | `repair_budget, maximum_*, offsets, started` | - | no | Operators |
| `try_repair` | 4360-4420 | 61 | DEAD (E13): move/reconnect/optimize repair with `validate_metric_vector` | `point, approved, moved, optimization_approved` | ... | `guarded_geometry`, `repair_kind`, `validate_metric_vector`, `optimize_patch` | no | Operators + Trial |
| `measure_private` | 4438-4534 | **97** | batched metric measurement of a cell list | `ready`, `measured_flags`, `result.reason`, stats, `certified` (shape) | `field_identity, campaign_edges, eligible_extensions, surface_target, phase` | `vote`, `trial_reduce`, `trial_import_samples`, `observe_transaction`, `surface_merit` | switch | Trial (+ Exchange donors) |
| `reserved_relocations` | 4553-4558 | 6 | count of relocated apex ids in the reserved range | - | `old, fresh, maximum_node, node_offset, construction_nodes` | - | no | Trial |
| `validate_private` | 4559-4617 | 59 | the private certificate of a candidate | `certified` (reset/restore), `ready`, `fresh_flags`, stats, `last_rejection`, `transaction_captured`, `result.reason` | `concurrent, approved, chosen, before, fresh, metric_cleanup, observe_rejections` | `geometric_change`, `measure_private`, `metric_trial`, `height_check`, `repair_kind`, `observe_transaction`, `vote` (conditional) | switch | Trial |
| `same` | 4579 | 1 | point equality (no capture) | - | - | - | no | Trial |
| `place_patch` | 4647-4742 | **96** | joint placement transaction | `dependencies, imported, old, reuse, before, approved, joint, cavity_inserted, fresh, placement_obstruction, certified, ready`, stats | `choice, concurrent, started` | `trial_maximum`, `vote`, `reserve_dependencies`, `measure_private`, `trial_import_cells`, `trial_reduce`, `observe_transaction`, `guarded_geometry`, `validate_private`, `reserved_relocations` | yes (directory) | Operators + Exchange + Trial |
| `movable_star` | 5036-5042 | 7 | apex movability by physical components | - | `geometry, choice.geometric` | - | no | Trial |
| `admitted_cells` | 5072-5079 | 8 | cell count of a mover cover | - | `old, candidates, payload` | - | no | Trial |
| `validate_joint` | 5184-5187 | 4 | selects the `validate_private` variant | (via validate_private) | `split_refit, compound_refit, choice` | `validate_private`, `reserved_relocations` | switch | Trial |
| `optimize_repair` | 5410-5439 | 30 | the bounded coupled optimizer step of a repair | `approved, repair_approved, moved, optimization_approved, ready`, stats | `coordinate_only, repair_work, chosen` | `trial_maximum`, `reserve_optimization`, `trial_import_cells`, `observe_transaction`, `guarded_geometry`, `optimize_patch`, `vote`, `release_optimization`, `validate_private`, `note_ladder` | switch | Operators + Trial |
| `geometry_counts` | 5497-5520 | 24 | count of geometry-failing faces/cells | stats, `atlas_budget` (CheckTime) | `geometry, options.surface_tolerance` | - | no | Trial |
| `admit_coverage` | 5594-5599 | 6 | admission of retained coverage bytes | `batch_bytes`, `retained_coverage` | `old, fresh` | `admit` (local in private mode) | switch | Control |
| `used` | 5887-5890 | 4 | IIFE: join Allreduce under a JoinWait scope | - | `private_work_used` | - | yes (direct `CPassiveComm::Allreduce`) | Control |

Unnamed predicate lambdas longer than 2 lines: `physical_edge` `any_of` (4077-4083, 7 lines), `boundary_edge` (4621-4623).

Lambdas above 15 lines: `plan_choices` 36, `boundary_replacement_ready` 24, `evaluate_candidate` 1476, `geometric_change` 40,
`optimize_patch` 29, `try_repair` 61, `measure_private` 97, `validate_private` 59, `place_patch` 96, `optimize_repair` 30,
`geometry_counts` 24. `note_ladder` is exactly 15.

Lambdas above 80 lines and their sub-jobs:

- `evaluate_candidate` (4108-5583): sub-jobs E01-E30 in section 1.6. Non-lambda blocks inside it that are themselves above
  80 lines: E13 (86, dead), E15 (115 incl. `measure_private`), E21 (278), E22 (83), E24 (220), E28 (85).
- `measure_private` (4438-4534):
  1. reset flags, copy `frozen_reuse`, `vote` (4440-4446);
  2. `count=trial_reduce(cells.size(),MAX)`, shape accumulator init (4447-4453);
  3. per measurement batch (4454-4530): query build + `MetricEdgeRequests` + `vote` (4458-4466); donor import with
     halving retry `for(;;)` (4467-4476); `FrozenField` + optional surface-planning sub-patch (4477-4498); per-cell
     measure and accumulation (4499-4523); counters (4525-4527);
  4. surface merit, reason, return (4531-4533).
- `place_patch` (4647-4742):
  1. participation gate `trial_maximum(search)`, work/time guards (4648-4652);
  2. features + `directory->Lookup` + oversized downgrade + `vote` (4653-4666);
  3. `directory->Fetch`, version check, swap `dependencies/imported/old`, rebuild `reuse` + `vote` (4667-4683);
  4. `reserve_dependencies` + conflict/deferred handling + `vote` (4684-4693);
  5. re-measure old into `before` + `vote` (4694-4699);
  6. donor import + allowance (4700-4709);
  7. `ReconstructHeightStar` or `PlaceSplitPatch` + counters + `guarded_geometry` + `vote` (4710-4732);
  8. work accounting, `validate_private`, approval/restore (4733-4741).

---------------------------------------------------------------------------------------------------

## 4. THE CANDIDATE PATHS

Shared head of every path: B05 (`pick`) -> B30/B31 -> B32 flags. Shared slot infrastructure: S01, S04-S10, E01, E05,
E10, E11, E14, E15 (old measure), E17 (`validate_private` first call), E18, E30.

### (a) Collapse (`coarsening && !repairing`)
1. Selection: B05 `pick` (`FilterCollapse`), or batched `selection.Batch` (B35c, when `candidate_batch>1 && !bad_field`).
2. Planning (multi-rank, not curved/composition): `plan_choices` -> `directory->PlanBatch` (B34) via B35c or B36.
3. Work admission: B35d or B36 (3885-3890).
4. Stars: batched B35e (no LookupBatch when planned; FetchBatch) or single S03a (planned: take `candidates.at(0)`; else LookupBatch + Fetch).
5. Curved surface collapse correction: S03c (`directory->CorrectCollapse`).
6. Reservation: batched B35h (`ReserveBatch`) or single S03d (`reserve_dependencies`).
7. Trial: S13 -> `evaluate_candidate` (private when `surface_target && !observe_rejections`): E09 collapse operator
   (`CollapseOutsideTolerance`/`ReconnectCollapse`/`CollapseVertex`) through `guarded_geometry` (-> `geometric_change`
   is NOT called by guarded_geometry; it is called in `validate_private`); E15 old measure (only if `metric_needed`); E17
   `validate_private(..., construction_kind=Coarsen)`; E20-E24 run only their participation collectives (all false); E27 loop
   breaks at trial 0.
8. Contact: S15 (curved surface edit or corrected collapse).
9. Final reservation: S16 (concurrent or corrected).
10. Commit: S17-S22 (parents layout keeps cells without `edge[0]` at 5719-5723; `result.collapses.push_back`).

### (b) Interior split (`!coarsening && !repairing`, no physical edge)
1. Selection: B05 `pick`, or batched `selection.Batch` (only when not curved/composition/split lanes); single mode with
   global priority + owner election (B36, composition/curved without concurrency) or `coalesce` (concurrent).
2. Work admission: B35d / B36.
3. Stars + reservation: B35e/B35h or S03a/S03d (with pass-deferred `continue`).
4. Trial: E07/E08 (composition: donor seed, `BalancedSplitPoint`); E09 `SplitEdge`, then `InsertEdgeCavity` (sets
   `cavity_inserted`); E17 `validate_private(Refine or GeometryRefine)`; on failure (composition/geometric) the joint
   machinery: E20 `place_patch` with a height apex or plain placement; E22 apex search + `place_patch` attempts.
5. Contact: S15 only if `curved && surface_edit` (interior edit can still change skin).
6. Final reservation: S16 if `concurrent`.
7. Commit: S17 resolves `point` from `fresh` (5689-5690); S19 parents (each old cell twice, or `i%old` if a cavity was inserted); S22 `result.splits.push_back({edge,point})`, child/parent ratio.

### (c) Surface split / curved (`!coarsening && !repairing`, `physical_edge` or curved geometry)
Same as (b) plus:
- E07 crease classification (`Binary64SegmentFreedom`) and `incompatible_crease`;
- E08 `centralPosition` and the retry at the central point in E09 (4276-4284);
- E18 `boundary_edge`, `flat_edge`, `strip`, `wallApices`, `wallSplit`;
- E21 strip search (`SplitSurfaceStrip`, extra edge split, frames with a `Reconnect` fan, normal moves);
- E23 boundary placement tensors; E24 growth loop (`RefitWallSplit`, `ReconstructWallSplit`, `ReconstructSplitPatch`,
  `RepairMetricConnectivity`, dependency growth);
- S15 `directory->Clearance` for a changed skin; S17 parameter edits.

### (d) Repair (quality/size/geometry; `repairing`)
1. Selection: always batched. B35b slot share (debt-proportional), B35c `selection.RepairBatch`, `coalesce`, `plan_choices` (no planning).
2. Work admission: B35d.
3. Stars: B35e `LookupBatch(...,repairing)` (coordinate stars), `FetchBatch`; B35f curved support expansion.
4. Reservation: B35h `ReserveBatch` with priorities.
5. Private budgets: B37 (`private_optimizer_allowance`, `private_work_reserved`).
6. Private trial per slot: S02 (local `!chosen` skip) -> S12 -> `evaluate_candidate` with `private_repair=true`:
   E09 (no operator), E15 old measure, E17 probe `validate_private(false,...)`, E18-E24 skipped (local false), E25
   fallback setup (metric move centre), E27 ladder (`MoveVertex` x3, `optimize_repair` at trial 3, `CollapseVertex`/`Reconnect`),
   E28 geometry-repair splits, E29 size-repair optimizer. Then S12 coverage admission, `ExportParameters`,
   `boundary_replacement_ready`, and storage into `repair_proposals[slot]`.
7. Join: B40a/B40b (work ledger, deadline, `vote(private_failure)`).
8. Contact: B40c/B40d `ClearanceBatch`.
9. Final reservation: B40e/B40f `ReserveBatch`.
10. Commit: B40g-B40l (routing, exchange, publish, receipts, selection update, `measure(batch_changed)`).
The collective "repair" variant no longer exists. Every repair slot is private (S12 runs whenever `repairing`), and
the `batched&&repairing` branches in S22 (5789, 5814, 5850-5856, 5871) cannot run, because S12 always ends the slot
with `continue` or `break` before S13.

### Shared steps and near-duplicates

| near-duplicate group | copies (lines) | difference |
|---|---|---|
| Final footprint completeness/version check | S16 5658-5667; B40c 5908-5918 | messages; batch loops over proposals |
| Routing build (removals/additions/blocked, `PublishMetrics`, physical faces, coordinate version bump) | S19 5711-5753; B40g 5979-6022 | per-slot parents layout has 3 cases (coordinate, inserted/repair/corrected `i%old`, split/collapse rule); batch has 2; batch also merges coverage and parameter edits |
| Three `world.exchange` calls | S19 5756-5764; B40h 6038-6048 | single baseline + one reason vs. accumulating baseline + three reasons |
| `directory->Publish` | S20 5770-5771; B40j 6056-6057 | coverage source, `repair_support_changes` flag (`repairing` vs `true`) |
| Retire/drop/checkpoint/Block/accepted counters | S22 5799-5847; B40l 6068-6101 | batch adds quality ladder/churn; per-slot adds `result.splits/collapses` and child ratio |
| `FinishRepairs`/`TouchRepairSupport`/`Touch` | S22 5860-5862; B40l 6105-6107 | per-slot adds `ServiceClaimedVertex` |
| Rejection bookkeeping (`last_metric_rejection`, `last_rejected_feature`, centroid, `last_rejection`, `record_rejection`) | `refuse` 3648-3650; `validate_private` 4594-4599; S18 5693-5701; B40g 6016-6020 | message prefixes |
| Lookup -> Fetch -> version check -> swap `dependencies/imported/old` -> rebuild `reuse` -> reserve -> re-measure old | `place_patch` 4653-4699; E21 4776-4819; E24 5309-5359 | E24 merges payload instead of replacing it; the conflict policies differ slightly (4688-4691 vs 4811-4813 vs 5347-5350) |
| Dependency merge with owner/version check | B35f 3813-3817; `place_patch` 4675-4679; E21 4797-4801; E24 5329-5334 | throw messages |
| Reservation conflict handling (`PassDeferred` / "conflicting dependency reservation") | 4687-4692; 4810-4814; 5347-5351; 5673-5677 | which flags are cleared |
| LookupBatch + FetchBatch of vertex stars | E22 5054-5058; E22 5099-5103 | features |
| Donor import -> `AssociatedExtensions` -> `observe_transaction("support_donors")` -> `FrozenField(...)` -> `FieldAccounting` | 4228-4231; 4349-4351 (dead); 4484-4486; 4713-4716; 4832-4835; 5214-5217; 5423-5426; 5533-5536 | identical constructor argument lists |
| Re-measure old after expansion + `vote` | 4694-4699; 4816-4819; 5356-5359 | |
| Neighbour-centroid move proposals (steps 1, .5, .25) | `try_repair` 4366-4397 (dead); E27 5457-5474; E25 5375-5380 | metric centre vs plain centroid |
| Reconnect over all cavity nodes | `try_repair` 4399-4409 (dead, max 16); E27 5475-5481 (max 16 via `trial<19`) | |
| Optimizer step then validate | `try_repair` 4411-4417 (dead); `optimize_repair` 5422-5436; E28 5560-5569 | |
| Global proposal work admission | B35d 3729-3734; B36 3885-3890 | the same message text |
| Work-budget guard inside a trial loop | `place_patch` 4649; E21 4962-4964; E24 5142-5146 | |
| Time-budget guard | B02 3296; B08 3352; B24 3537; `place_patch` 4650; E21 4965; E24 5147; S12 5585 (local); E28 5539 (local); B40b 5894 | collective vs local |
| `refused_receipts` push for pass-deferred | `plan_choices` 3670-3675; B35h 3848-3853 | |
| `service_frontier` on refused receipts | 3681-3684; 3833-3836; 3854-3857 | identical |
| "cells containing both edge endpoints" scan | 4673, 4908, 4930, 5002, 5049, 5074, 5124 | |
| wall-face apex scan | E18 4634-4643; E20 4747-4753 | |
| Feature of a candidate `repairing ? choice.feature : Feature{{e0,e1,0},2}` | 3739-3740, 3945, 4065, 4596, 5696, 5784 (+ coarsening variant `{{e1,0,0},1}` 3663, 3740, 3945) | |
| Lane switches | B19 3440-3448; B20 3449-3457 | mirror images |

---------------------------------------------------------------------------------------------------

## 5. COUPLING

### 5.1 Lockstep (every rank makes the same sequence of collective calls)
- Round head (B02-B31): each `world.maximum`, `vote`, `admit`, `enter_phase`, `observe_progress` etc. runs under
  replicated conditions. Several conditions are SHORT-CIRCUIT chains that contain collectives (3291 `surface_ready()`,
  3376, 3387, 3395-3396, 3403-3404, 3429-3431, 3440-3441, 3458-3459, 3544-3549, 3565). The number of collectives depends on
  the evaluation order. This keeps lockstep (the operands are replicated) but changes `world.collective_calls`, which
  feeds `stats.tracked_calls` in `finish()` and `private_tracked_collectives`.
- Slot loop (non-repair): the trip count is the global `slots` or 1. Ranks without a candidate still take part with empty
  inputs (`choice.active==false`, empty features). Every slot `continue` (3941, 3949, 3967, 3999) is decided by a
  collective (`world.maximum`). S03d's `admitted` is a global value.
- `evaluate_candidate` (collective mode): every collective is guarded by `trial_maximum(...)` of a local condition,
  including loop headers: E21 `trial<originalTrials+normalTrials && trial_maximum(strip && !approved)`; E22 attempts;
  E24 growth and frames (`frames` computed from `trial_maximum`), `close_first`, `break`s at 5302/5306/5321/5343/5353;
  E27 `trial_maximum(fallback && !approved)` break and `trial_maximum(participating)` continue. Directory calls inside
  (`Lookup`, `Fetch`, `LookupBatch`, `FetchBatch`, `Reserve` via `reserve_dependencies`) are always made with empty
  arguments by non-participants (4654-4666, 4777-4790, 5099, 5309 is unconditional inside the growth iteration).
- Conditional votes guarded by option flags: 4174, 5581, 5692 `if(options.transaction_observer && !vote(...))`.
  These assume `options.transaction_observer` is set identically on all ranks (contrast `observe_rejections`, which is
  agreed with `world.maximum` at 1629). 4613 `if(observe_rejections || (!repairing && !coarsening))ready=vote(observed);`
  and 5355 `if(capture && !vote(failure))` are replicated.
- Mixed collective types in one expression: 4733 `stats.global_work+=concurrent ? trial_reduce(SUM) : trial_maximum(...)`,
  5142 / 4962 / 4707 `concurrent ? trial_reduce : 1`.
- Private paths: S12 (repair) and S13 (surface collapse) must make NO collective. S13 enforces this by comparing the
  `World` counters and `stats.global_work` (5630, 5636). It sees only calls counted by `World`: direct
  `TimedAllreduce`/`CPassiveComm::Allreduce` calls are invisible to it. `CNativeDistributed3D.cpp` uses both kinds
  (24 `world.exchange/maximum/elect` sites and 6 direct `Allreduce` sites), so a directory call would normally be seen
  through its `World` part. S12 does not enforce it; it only accumulates the delta into `stats.quality.private_tracked_collectives` (B40a).
  Private safety rests on `joint==false` for repair and collapse, so E20-E24 skip their directory calls.
- After the private slots: B40b `used` (direct Allreduce) is the first collective after the slot loop. It is the join
  point, and its position after the local `break` paths is essential.

### 5.2 State shared between the slot loop and the commit part
- Per-slot path: the evaluation and the commit live in the SAME slot scope. Shared: `failure` (S05, voted again in
  S17/S19/S22), `old/fresh/point/approved/chosen/certified/...`, `dependencies/imported` (S16 reserve, S19 owners),
  `pass_deferred` (S19, S21), `collapse_corrected` (S15, S16, S19), `result.reason` (written in E, appended in S14, saved
  in `pending_rejection`, overwritten by S15/S16, restored in S17 5683).
- Repair path: S12 moves slot state into `repair_proposals[slot]`; B40 reads only `repair_proposals`, `private_*`,
  `choices`-independent data. B40e moves `proposal.imported` into `final_inputs` and B40f moves it back.
- Round-to-slot: `choices`, `dependency_batch`, `imported_batch`, `reserved` (moved from per slot; the batch objects are
  left in moved-from state), `concurrent`, `batched`, `planned_coarsening`, `timed_lane`, `bad_field`, `round` (ID ranges).
- Function-level carriers across all blocks: `ready` (read after `validate_private`, `measure_private`, `place_patch`
  as `if(!ready)return false;` and in S03e), `batch_bytes` (B35a/B35e/B35f/S12/B40i; reset only in B41),
  `stats.global_work` (B35d, B36, B37, S03c, E11, E21, E24, `place_patch`, B40b refund).
- `source` cavity: `source.SetCavity(&imported.cells)` (S08) stays active until slot exit, so the collective
  `measure(changed,...)` in S22 (5857), `world.exchange` in S19 and `Publish` in S20 run WITH the slot cavity set. The
  batch `measure(batch_changed,...)` in B40l runs with no cavity. `CavityMetricSource::ImportCells` uses `cavity`
  (CNativeMetricTransport3D.hpp:271), so the cavity lifetime may affect `measure` results.

### 5.3 Early exits that skip another unit's bookkeeping
- Round-level `continue` (B09-B27) skips B41: `round` is NOT incremented and `batch_bytes` is NOT reset. Required, because
  "A final census/phase transition consumes no additional operator round" (3600) and `round` feeds the ID ranges and the
  window test.
- `break` at B29 skips B30-B41 and goes to the post-loop verdict.
- Slot `continue` at 3941/3949/3967/3999 skips the evaluation and all commit/receipt work for that slot; only 3999 and
  3965 call `service_frontier` before skipping.
- S12 `break` (5586 deadline, 5613 failed evaluation, 5625 storage failure) skips the remaining private slots, but B40
  still runs its join; `vote(private_failure)` in B40b then ends the round.
- `return finish()` from inside the slot loop (S03a/b/c/d/e, S13-S22) skips the remaining slots, B40, B41, and the
  `batched&&repairing`-style deferred updates; `finish()` itself is collective, so all ranks must leave at the same point
  (guaranteed by the preceding vote).
- `plan_choices` returning false leaves `refused_receipt_count` unserviced; the caller finishes.
- `evaluate_candidate` returning false in collective mode (S13 `else if(!evaluate_candidate())return finish();`) skips
  S14-S22 bookkeeping, including `++stats.rejected` and `service_frontier` for that slot.
- S03e: when no rank reserved and none deferred (`admitted==0`), `ready=false` and the whole `Adapt` returns `finish()`.

---------------------------------------------------------------------------------------------------

## 6. IDENTITY RISKS FOR A MECHANICAL EXTRACTION

1. ID ranges depend on `round` and `slot` (4041-4046:
   `epoch=round*world.size+rank`, `token=epoch*candidate_batch+slot`, `offset=token*MaximumReplacementCells`,
   `node_offset=token*node_stride`). Any change in where `++round` happens (B41 only), any renumbering of slots
   (batched: index in `choices` after `coalesce`/`plan_choices`; single: 0), or any change of `construction_nodes`
   changes every new cell/node id and therefore the mesh. The shadowing `slot` inside `measure_private` (4454) must not be
   confused with the outer `slot`.
2. Order of evaluation inside one statement / short-circuit collectives: see 5.1. Examples:
   3881 `if(!vote(failure) || !coalesce(choices) || !plan_choices())`; 3383 `!observe_progress(...) || !enter_phase(...)`;
   3429-3431; 5646-5647 `(curved && world.maximum(surface_edit)) || world.maximum(coordinate_only && approved) || world.maximum(collapse_corrected && approved)`;
   5655 `(concurrent || world.maximum(collapse_corrected)) && world.maximum(approved)`; 4312
   `trial_maximum(...) && !admit(...)`; 5373 `composition && trial_maximum(...)`; 5411 `... || !trial_maximum(...)`.
   Hoisting any term into a separate `const bool` evaluates the collective eagerly and changes the `World` counters
   (`stats.tracked_calls`). Also `ready=vote(...)` assignments inside conditions (3315, 3665, 3680, 3720, many in E).
3. `TimedAllreduce` vs `world.maximum`: the first bypasses `World` counters. Swapping them (for example, using a common
   "reduce" helper) changes `stats.tracked_calls`/`private_tracked_collectives` and S13's private check.
4. RAII destructor order:
   - `phase_scope` (3286) and `lane_timer` (3304) outlive `return finish()`. Because `stats` is a reference into `result`,
     and `finish()` ends with `return std::move(result);`, the lane timer's increment lands in the moved-from `result`
     after the return value was built. Moving `finish()` outside the round scope adds the last partial round to
     `surface_seconds`/`interior_seconds` and moves `finish()`'s proof phase out of the round's PhaseScope (F3 ledger).
     The same applies to every `Timer` block that contains `return finish()` (3319, 3536, 3569, 3603, 3713, 3727, 3737,
     3839, 3862, 3944, 3976, 3989, 5709, 5769, 5787) and `continue` (3989-3999).
   - `CavityScope` (4076) must live until the end of the slot body, AFTER the commit (see 5.2). If `evaluate_candidate`
     is extracted together with the cavity scope, `measure(changed)` in S22 would run without the cavity.
   - `FieldAccounting` (4231, 4716, 4835, 5217, 5426, 5536) counts field queries in its destructor, also on exceptions.
     At 5425-5427 the `Timer` is declared after `proposal_field`/`accounting` and destructs first.
   - `SU2NativeTiming3D::Scope candidate_timer` (4560) spans all of `validate_private` including nested
     `measure_private` timers; 4344-4427 subtracts nested reconstruction time explicitly
     (`stats.metric_seconds-=stats.reconstruction_seconds-reconstruction_before`).
   - `CLocalFailure` has no destructor side effect; the risk is SHARING: the slot `failure` (4026) is captured by many
     lambdas and voted again after evaluation (S17 5692, S19 5755, S22 5849/5869). A copy instead of a reference changes
     which failures reach later votes.
   - `private_repair=true/false` (5588/5612, 5631/5634) is not RAII; an exception escaping between them is caught locally
     in both places, so the flag is reset. An extracted version must keep both resets on all paths.
5. Shadowed variables:
   - `complete`: round (3307) vs B35b `const bool complete` (3705) vs B35f `bool complete` (3806) vs `auto complete=directory->FetchBatch` (3826).
   - `original`: function parameter vs B35f `auto& original=dependency_batch.candidates.at(...)` (3804).
   - `reserved`: round `ReservationBatch` (3635) vs `optimize_patch` `const auto reserved` (4316) vs `bool reserved` in
     `place_patch` (4684), E21 (4806), E24 `const bool reserved` (5344).
   - `slot`: outer vs `measure_private` inner loop (4454).
   - `failure`: `CLocalFailure failure` at 3328, 3389, 3398, 3524, 3551, 3596, 3608, 3620, 3654 (plan_choices), 3698
     (batched), 3879, 4026 (slot, shared), 5900 (post-slot); each extracted function must bind the right one.
   - `reuse`: 4353 (dead path) vs 4535 vs `measure_private` local copy 4444.
   - `nodes`: E25 (5365) vs E28 `std::map<Id,Node> nodes` (5495) vs E21 local (4835) vs `try_repair` (4361).
   - `retained`: B37 (3906) vs `validate_private` `const auto retained=certified` (4562); `used`: B30 (3604) vs IIFE (5887)
     vs E24 `std::set<Id> used` (5238); `removed/added/failures/parameter_edits`: slot (5684-5707) vs B40 (5973, 6037);
     `candidate`, `target`, `donors`, `features`, `joined`, `more`, `local`, `count`, `edges`, `participating`,
     `measured` are reused names at many depths.
6. Lambdas capturing by value: none (all are `[&]`; `fell` and `same` capture nothing). `trial_reduce` is a generic lambda
   (`auto value`): its return type follows the argument (`uint64_t`, `size_t`), which matters for overflow if retyped.
7. Indentation: the slot body (3925-5873) and the `evaluate_candidate` body (4109-5582) are not indented relative to their
   openers; `if (streamed) {` (4436) opens a block that closes at 5579. Tools that infer scope from indentation will cut
   at the wrong places.
8. `continue`/`break` as goto-like control:
   - Round `continue` (3362-3586, 17 sites) restart the round without `++round`.
   - Slot `continue` at 3941, 3949, 3967, 3999 (inside a reservation `Timer` scope and an `else if`), 5626.
   - Slot `break` at 5586, 5613 and 5625 (the last inside a `catch` block).
   - Inner-loop `break`/`continue` inside `evaluate_candidate` (4396, 4401, 4408, 4473, 4582, 4637, 4754, 4762, 4880, 4886,
     4897, 5064, 5097, 5244, 5246, 5284, 5302, 5306, 5321, 5343, 5353, 5443, 5448, 5506, 5511, 5539).
   Extracting a slot body into a function needs a tri-state result: NextSlot / LeaveSlotLoop / FinishAdapt (plus
   "return finish()" which must still be called inside the original RAII nesting, see 4).
9. Exceptions:
   - The pattern `try{...}catch(const std::exception& e){failure.Set(2,0,e.what());} if(!vote(failure))...` is everywhere;
     the try must keep exactly the same extent, or a throw will skip or add statements before the vote.
   - Not caught before leaving `Adapt`: 3849 `throw std::logic_error("Native construction deferrals exceed frontier bound.")`
     in B35h (outside any try). In the collective path (S13 `else if(!evaluate_candidate())`), exceptions not caught inside
     `evaluate_candidate` propagate out of `Adapt`, e.g. `ValidateMetricChange`/`ValidateMetricCleanup` in
     `validate_private` (4574-4576, no try) and `PreservePassingFirstHeights`/`height_check` in `guarded_geometry` when it
     is called outside a try (for example 4730 is inside a try, 5254 is inside a try, 4123 itself has none). The
     swap-and-rebuild steps (4680, 4802, 5338) are inside tries. In the private paths all of these are caught
     (5589-5611, 5632).
   - `boundary_replacement_ready` rethrows `bad_alloc` (4105); its callers catch `std::exception`, so the rethrow is
     caught one level up.
   - `refuse` can throw after incrementing stats (3643-3645).
10. Mode invariants that an extraction may rely on (verified in source, not by test):
    - `repairing` is constant inside the slot loop (only B25 writes it inside the loop); repair slots always end in S12.
      Hence inside S13-S22 `repairing==false`, and the `batched&&repairing` branches (5789, 5814, 5850-5856, 5871),
      `repairing ?` ternaries (5696, 5699, 5784, 5816, 5819-5823) and `repair_approved` paths (5717, 5835, 5578, 5362)
      are dead.
    - `streamed` is always true (4290 returns on `!ready`, 4294 `streamed=ready`), so E13 (4343-4428 incl. `try_repair`),
      4430-4435 and `validate_metric_vector` callers are dead, but E11's `reserve_optimization(false)`,
      `trial_maximum(repair_budget>repair_work)` and E14's `release_optimization()` still make collectives in
      collective mode and must stay.
    - `selection.geometry_repairs`/`size_repairs` imply `repairing` (set only in B14/B15 under Improvement; cleared by
      `ChangePhase`, which runs on every exit, including `begin_length_generation`).
11. Wall-clock dependence: S12 local deadline (5585), E28 local time break (5539), `optimize_patch` remaining time
    (4318-4320), `atlas_budget.CheckTime()` (5503). Identity tests must run with time budgets far from binding.

---------------------------------------------------------------------------------------------------

## 7. PROPOSED EXTRACTION ORDER FOR THE LOOP

Each step compiles and keeps mesh and counters identical. Check each step with the existing exact saved-state gates.

1. **RoundState / SlotState structs, no code motion.** Replace the round-level declarations (B04 3306-3307, B32
   3632-3641, B37 3902-3904, B38) with one `RoundState` struct, and the slot-level declarations (S01, S05, S07, S08 data)
   with one `SlotState` struct, still declared at the same points (keep `CavityScope` and the timers as separate locals
   in the same order). The lambdas keep `[&]` and now refer to `round_state.x`. Risk: low; only renaming.
2. **Move `RepairProposal` to namespace scope** (3892-3901) and give it a fill function from `SlotState` (S12 5614-5624).
   Risk: low.
3. **Round-head Control as a decision function.** Extract B02 and B07-B29 into
   `RoundDecision DecideRound(EngineContext&, RoundState&)` that returns {Proceed, Restart, Finish, Break}. The caller
   does `continue` (without `++round`), `return finish()` (inside the same `phase_scope`/`lane_timer` scope) or `break`.
   Keep every short-circuit expression verbatim; one block per helper function (B09, B13-B23, B25-B28 as separate named
   functions). Risk: medium (short-circuit collectives, `refill` coupling between B05/B07/B10).
4. **Acquisition (Queues + Exchange).** Extract `refuse`, `plan_choices` (B33, B34) to member functions of a
   `RoundAcquisition` object, then B35 sub-blocks a-h and B36 as one function each, returning a status that the caller
   turns into `return finish()`. Keep `TimedAllreduce` vs `world.maximum` choices. Risk: medium (3849 uncaught throw,
   `refused_receipts` sequencing, `ready` writes).
5. **Repair batch commit (B40).** Extract B40a-B40l as functions taking `RoundState&` (`repair_proposals`, `private_*`);
   B40b join stays first. Risk: medium-low (pure sequence, no RAII except Clearance/JoinWait scopes).
6. **Per-slot commit (S14-S22).** Extract as functions taking `SlotState&`, called INSIDE the slot body so that
   `CavityScope` and the slot `failure` stay alive. Do not remove the dead `repairing` branches in this step; remove them
   in a separate step with its own gate run. Risk: medium (`result.reason` save/restore order; `pending_rejection`).
7. **Slot acquisition (S02, S03a-e).** Extract with a tri-state slot result {Next, Proceed, Finish}. Risk: medium
   (`continue` inside the reservation `Timer`, global `admitted`).
8. **Trial switch object.** Replace S04 lambdas with a small `TrialComm` object (`private_repair` flag + `World&` +
   `source`) with methods `Maximum`, `Reduce`, `ImportCells`, ... that keep the exact call targets. Risk: low-medium
   (the `trial_reduce` template type).
9. **`evaluate_candidate` leaves first.** Move the small pure lambdas to functions: `guarded_geometry`, `repair_kind`,
   `geometric_change`, `reserved_relocations`, `movable_star`, `admitted_cells`, `geometry_counts`, `incompatible_crease`,
   `note_ladder`, `optimize_patch` (split counters into a `AccumulateOptimization(stats,ops)` helper),
   `reserve/release_optimization`. Risk: low-medium.
10. **`measure_private` and `validate_private`.** Split as in section 3 (4 + 3 sub-functions). Risk: medium (`ready`,
    `certified` reset/restore at 4562-4563/4614, conditional vote at 4613).
11. **Joint machinery.** `place_patch` (8 sub-functions, section 3), then E21 strip search (seed generation, frame
    building, trial loop), E22 apex search, E24 growth loop (frame trial, connectivity repair, obstruction choice, growth
    transaction). Fold the three Lookup/Fetch/verify/swap/reserve/re-measure copies into one `ExpandCavity` only in a
    later step with its own gate, because the conflict policies differ (4688-4691 vs 4811-4813 vs 5347-5350).
    Risk: HIGH.
12. **Repair ladders.** E25-E29 as functions (`PrepareFallback`, `MoveCentre`, `OptimizeRepair`, `FallbackLadder`,
    `GeometryRepairSplits`). Risk: medium.
13. **`evaluate_candidate` shell.** What remains (E01, E05, E07-E10, E14-E18, E20, E30) becomes an
    `EvaluateCandidate(SlotState&, TrialComm&, ...)` sequence of calls. Risk: medium.
14. **Dead-code removal** (E13/`try_repair`, 4430-4435, `batched&&repairing` in S22, repairing ternaries in S17-S22),
    each as a separate step with its gate run, keeping the 3 collectives of E11/E14. Risk: low-medium once 10 is done.

Riskiest step: **step 11 (the joint machinery: `place_patch`, strip search E21, growth loop E24).** Reasons:
(i) it contains the densest lockstep protocol: loop headers, `break`s and frame counts are all decided by
`trial_maximum`, and directory calls are made with empty arguments by non-participants; (ii) it mutates the slot's
`dependencies`/`imported`/`old`/`reuse`/`before`/`approved`/`joint`/`certified` in place through shared references, and
`source`'s cavity pointer follows `imported.cells`; (iii) it uses new-node ids `maximum_node+2+i+node_offset` and node
renaming (5241-5253), which change the mesh if the order changes; (iv) it is the least covered path (composition/curved
surface splits only), so the saved-state gates may not reach every branch. The second riskiest is step 3, because of the
short-circuit collectives and the rule that `continue` must not advance `round`.
