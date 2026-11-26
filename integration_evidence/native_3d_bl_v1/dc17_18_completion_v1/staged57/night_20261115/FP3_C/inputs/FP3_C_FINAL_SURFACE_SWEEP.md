# FP3-C: one more surface-collapse sweep of the core after the volume work of the last pass (mode S), with an adoption rule

Author: reviewer, 2026-11-26 (user direction of 2026-11-26: the useful part of an old-engine function is put
into the new core; reviewed by Codex, source facts by Codex). For the main agent. The user sends it; it is a
user order. Copy this file with its SHA256 into `R/night_20261115/FP3_C/inputs/`.
Code base: `f7447473e5` ("FP3-B: accepted; no exchange at the empty collapse exit"), build676.
Short names: R, N, "mode S", OFF, CORE, "worker stage", engine and core file roots as in the order FP3-A
(your copy in `N/FP3_A/inputs/`), called "A"; "B" = the order FP3-B and its record
`N/FP3_B/accepted_reference.json` (the S reference in force); "step C" = the surface collapse of the core
(`SurfaceCollapseWork`, `native_local/CNativeLocalCore3DSurface.cpp:969-1052`); "the sweep" = the new call of
section 2.

Scope. One production change in the pass driver of the core, active only in the LAST pass of a core run with
surface collapses (mode S). In OFF, in CORE, in each pass that is not the last, and in the lab control with
the probe value `2`, expect the behaviour of build676. It changes the mesh of mode S: the acceptance is the
two-run rule for S. The item has an ADOPTION RULE (section 4, step 3): when the sweep accepts too few
collapses on the real case, the change is taken out again. That outcome is not a failure.

## 1. Why

Since A the engine makes no construction collapse after the core. Before, that engine pass removed 646 / 446
surface points of smooth sheets in the two events (your engine probe of A). The useful part of it must come
from the core. Step C runs one time for each pass, BEFORE the volume work of that pass
(`CNativeLocalCore3DPass.cpp:493-497`); nothing runs it after the volume work of the last pass. Your core
probe of A (`NATIVE3D_FP3_HANDOVER_SURFACE`, checkpoint `last_pass_pre_shift`): 1,868 / 936 short surface
edges pass `PrepareSurfaceCollapse` after the volume work. The probe did not run the later checks of step C
(remote screen, contact, samples, entity, bound). Reviewer study:
`reviewer_handoff/scripts/s3/study_surface_coarsening_in_core.md`.

| Assumption | State |
|---|---|
| With the option false the pass is bit-identical to build676 | from the form of the change. Tested by step 1 |
| How many collapses the sweep accepts | NOT KNOWN. It may be near zero. The historical accepted share of prepared directions on the edge counts of the probe gives the illustrative figures 630 and 300; they are no bounds and no criteria. Measured by step 2 (lab) and step 3 (S run 1); the adoption rule uses the measured count |
| The repair after the sweep leaves no more bad cells than today | NOT KNOWN. Reported (core `bad_out`, engine `bad_union`); the standing gates decide |
| The cost is small (about 1 to 2 s for each rank and 40 to 90 ms of bound for each accepted collapse) | an estimate from your logs. Reported |
| Two runs S give the same files | NOT KNOWN for this change. Gate of step 4 |

## 2. The production change

2.1 The option. A new `bool` in `CoreOptions` (default false; name it for what it says: a surface-collapse
sweep after the volume work). In `RunLocalCore` (`native_engine3d/NativeRemeshRun.inc`) set it for a pass only
when `surface && options.surface_collapses && pass == passes` AND `FP3Probe() != CollapseProbe::Engine` (so
the sweep stays OFF in the lab control value `2`, whose reference is the S entry of the FP2-P3 record). Apply
this last-pass condition by itself in each eligible worker stage, a normalization correction included. With
one scheduled pass that pass is the last. The condition of A that closes the engine collapse queue does not
change.
2.2 The sweep. In `Mesh::RunPass` (`CNativeLocalCore3DPass.cpp:483-507`), after `RepairRounds` and before
`Check()`, when the option is true:
1. `SurfaceCollapseWork` on the same surface and the same remote data of the pass, inside its own
   `SurfaceQueryScratch` (as the first call has it, `CNativeLocalCore3DSurface.cpp:1056-1057`);
2. ONLY when the sweep committed at least one collapse (an unconditional commit count: it must not depend on
   `stats.report` or on the timings): `MarkUnresolved()`, then `RepairRounds` one time. With zero commits
   skip both.
Steps L and D run one time, before the volume work, as today: this item adds only a C sweep after the
volume work. (The volume work keeps the physical surface; it can change the cavity and spoke tests that a
new L or D trial would meet. That is not in this item.) The rules of step C are not changed (each refusal,
the frozen border, the proof, the contact tests). What runs after `RunPass` (the exchange of the pass,
`VerifyPassContact`, the shift, `InstallLocalSurface`) is not changed.
2.3 Counters. The existing L / D / C census, the first-repair counters and their timings keep their values
at their original checkpoints (the legacy `seconds_quality` is measured through the FIRST repair). The full
pass elapsed time and the final `cells_out`, `bad_out`, `short_out` stay at their existing end-of-pass
checkpoints, so they show the state after the sweep. The sweep and its repair count into their own
statistics. Under `options.timings` print one new row for each rank and worker stage after the last pass:
`NATIVE3D_LOCAL_SURFACE_FINAL rank= stage= pass= candidates= prepared_directions= collapses= <one field for
each `SurfaceCollapseRefusal` value other than `None`>= metric_floor= bound_queries= c_sweeps= star_max=
v_limit= seconds= seconds_bound= bad_before= bad_after= repair_seconds= faces_before= faces_after=`.
- `candidates` = the `c_candidates` sum of the sweep over its internal sweeps; `prepared_directions` = each
  direction whose `PrepareSurfaceCollapse` returns `None`, also one that stays untried after the other
  direction commits; `collapses` = the committed removals; the refusal fields = the existing per-direction
  accounting, with the metric-floor subset apart;
- `seconds` and `seconds_bound` = the timings of the sweep; `repair_seconds` = only the second
  `RepairRounds` (zero when it is skipped);
- `faces_before` and `bad_before` are taken immediately before the sweep; `faces_after` and `bad_after`
  after the optional repair; `bad_*` = the cells with `Margin < SU2NativeLocal3D::BadMargin` (local count, no
  collective call).
- `c_sweeps` = that of the final C call; `star_max` = the local live-cell incidence immediately after the
  final C, before the optional repair, with the definition of the legacy surface row; `v_limit` = that of
  the second repair, or zero when the repair is skipped.
Print a complete row also when the sweep finds no candidate or commits no collapse: always the measured
before / after censuses and the sweep time. With no candidates the candidate, direction, refusal, collapse
and bound-query counts are zero. With candidates and no commit: the observed counts and timings, with
`collapses=0` and `repair_seconds=0`. Put the larger of the two query-scratch peaks into the existing pass
budget report.
2.4 Storage and time: verify in the R4 source check that the admitted bytes of the contact index and of the
query scratch cover a second build in the same pass, and write the result. No new timer operation, no field
in `Session`, `CellState` or a row that travels; `fixedstorage=4120` stays. Correct the comment at
`CNativeLocalCore3DPass.cpp:492`.
2.5 No other production change. No config option. The diagnostic variable `SU2_NATIVE_FP3_PROBE` of A keeps
its three values and their meaning.

## 3. Hidden unit cases (G1 does not change)

In the existing hidden step-C cases (`[NativeFP2C2]` and the hand-over fixtures): (a) with the option false
each existing case gives its pinned values; (b) a case where step C refuses an edge for a reason that reads
the volume (for example Shape or Validity), the volume work of the same pass removes that reason, and the
sweep collapses the edge: assert the mesh, the row of 2.3, and that the first step-C fields are those of the
run with the option false; (c) a border point is not removed and not kept by the sweep; (d) the option true
with no commit of the sweep: the written-back mesh, the point ids and the boundary associations are those of
the option-false result, and a complete row of 2.3 with `collapses=0` and `repair_seconds=0` is printed.

## 4. The work, the gates and the labels

A new build number and receipt folders under `N/FP3_C/`. One frozen binary pair. One owned heavy job; the host
preflight of R1 before each launch. Adapters: copies of the adapters of B (test code, R3) bound to this
order and the base `f7447473e5`; the runner rules of A and B hold (probe variable `0`, verified and
receipted; the one-key exclusion for G3 / C492; each S trial launched and sealed by itself). The copied
full-run prerequisites are replaced by seals of THIS order: S run 1 needs the passed steps 1 and 2; S run 2
needs the passed S run 1 (workflow and restart receipts), the complete sweep measurement and the sealed
disposition "Y >= 273". The S files of B are a historical comparison only.
The core lab (test code, R3): a copy of the single-trial campaign adapter of FP2-P2 with the evaluator of
FP2-C3, bound to the build, receipt and artifact pins of this order; replace its old C3 checkpoint
dependency (`c3_acceptance.json`, the 18 FP1 cohorts) by the prerequisites of this order. The same inputs
and observer settings as the corresponding FP2-P2 campaign trial, trace `0`, witness closure at its default
(on), probe `0`. The sweep choice is exposed and receipted in the TEST DRIVER only: it sets the option only
in the last pass (N6P: stage 0, pass 2; N1: stage 0, pass 1), and the effective choice is exported and
verified on each rank.
Lab rules with the sweep on: keep the legacy L / D / C census and the first-repair counters at their original
checkpoints; use `faces_after` of the row of 2.3 for the independent final-face census; keep the first-C face
and point closure checks on the legacy C row and require `faces_before - faces_after == 2 * collapses` for
the sweep; forward the existing collapse-child sink for BOTH C calls, so that the strict surviving-child
proof covers the sweep; apply the inherited alarm limits to the row of 2.3 (`c_sweeps <= 49`,
`star_max <= 1000`, `v_limit == 0`) and keep the legacy alarms on the legacy rows; keep the exact final positivity, contact, embedding,
strict proof and final `lt_1e_2` gates.

Step 0. One R4 source check of this text.
Step 1, the paths without the sweep (closure default, probe `0`):
1. G1 identical to the reference; the hidden cases of section 3.
2. G2 and GQ against the accepted P3 captures: equal.
3. The five shared checks as in A step 1 item 3; G3 and G4 as in B step 1 item 4.
4. The core lab with the sweep OFF: one N6P trial; `repeat_differences` against the qualified measurement
   that `N/FP2_P2/campaign/build672/N6P/trial1/case_summary.json` selects (verify that receipt and its
   measurement): an empty difference list.
Label: "FP3-C: default paths unchanged", or "... FAILED", or "... OPEN". Not evaluated (write it): the CORE
and OFF full workflows (they do not reach the sweep: no surface collapse in CORE, no core in OFF).
Step 2 (after step 1 passed), the cheap check: the core lab with the sweep ON, one N6P trial and one N1
trial. Gates: the lab rules above and exactly one complete row of 2.3 for each participating rank with the
expected stage and pass. Report: the row of 2.3 by rank and summed; hand-over faces, cells, `bad_out`,
`short_out` and the seconds of the last pass, beside the sweep-off trial of step 1 (N6P) and beside the
qualified measurement of `N/FP2_P2/campaign/build672/N1/trial1/case_summary.json` (N1, a historical
comparison).
Step 3, S run 1 with its isolated restart (after step 2: both trials qualified). Gates: all standing gates
of both events; the suppression gate of A and the ownership gate of B, unchanged; and the sweep measurement:
join each row of 2.3 by event, worker stage and rank to the validated worker packets of B's collector (the
row is printed before the seed of its packet); exactly one row with the expected last-pass number for each
participating rank of each worker stage; a duplicate, missing, malformed or misplaced row follows the
required-measurement and watchdog rules of A. Report beside the S pair of B: the rows of 2.3 by event, stage
and rank and summed; final cells, points, surface faces; core `bad_out` and engine `bad_union`; quality
seconds; final-proof seconds; native maxima with the host load.
ADOPTION RULE. Y = the sum of the integer `collapses` of the complete, sealed row set of S run 1, over every
worker stage of both events and all ranks. The old engine pass removed 646 + 446 = 1,092 smooth-sheet
points; the sweep is adopted when it gives back at least one quarter of that: Y >= 273. Seal Y and its
disposition before S run 2 can start.
- Y >= 273: go on with step 4.
- Y < 273 (with all gates of S run 1 passed and the measurement complete): the change is NOT adopted. In ONE
  commit revert the production change of section 2 and remove or revert the test-source changes that depend
  on it (the tree must build without the option and its statistics). Keep the implementation checkpoint in
  the history, and the adapters, receipts, logs and the report as evidence. FP3-A, FP3-B and the accepted
  reference of B stay in force. Publish the report
  with Y and the revert commit; S run 2 is "not evaluated"; the accepted reference of B stays in force.
  Label: "FP3-C: not adopted; the sweep gives Y collapses" (with the number). STOP. This is the planned
  result of a measurement, not a failure (R5 does not apply to it). A measurement failure or a watchdog
  cannot give "not adopted": it follows the failure rules.
Step 4, S run 2 (same pair, inputs, settings) with its restart: the gates of step 3, and the six files equal
in the two runs. Then they are the new S reference.

Acceptance (when steps 1 to 4 passed): publish `N/FP3_C/accepted_reference.json` (`workflows.S` in the form of
B) and append to `AGENTS.md` section 2a: "- Since FP3-C the S reference is the `workflows.S` entry of
`R/night_20261115/FP3_C/accepted_reference.json` (the core runs one more surface-collapse sweep after the
volume work of its last pass). It replaces the S entry of FP3-B for ordinary S runs. The FP2-P3 record stays
for OFF, CORE, G2, GQ, G3 and `saved_current`; its S entry stays the reference of the lab control value `2`."
Update the progress file, `HANDOFF_Codex.md`, `HowToProceed.md`, `Handbook/ADAPTATION_TIMINGS.md`. Label:
"FP3-C: accepted; final surface sweep in the core". Report `N/FP3_C/reports/FP3_C_<build>.md` and `.json`. Then
STOP.

Failures: the paragraph "Failures" of A holds for each required result gate of steps 1 to 4 (the lab gates of
step 2 included), with "the production change of FP3-C" for "the connected production change of FP3-A": a
result failure that stays reverts ONLY the change of this order, with its dependent test-source changes in
the same commit (the tree must build), as in the "not adopted" case; label "FP3-C: FAILED; no final surface
sweep". The rules R1, R3, R5, R6, R7, R8, R9, R10, R11, R12, R13 hold. Expected machine time: about 3 hours.

## 5. Not in this order

The removal of feature-curve points by step C (a later item with its own design). A sweep in a pass that is
not the last. A new run of steps L or D after the volume work. A change of a rule of step C, of the border
shift, of the engine, of the ownership exchange or of the final proof. A config option.
