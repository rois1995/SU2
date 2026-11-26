# FP3-A: no engine construction collapse after the surface core (mode S), with two probes

Author: reviewer, 2026-11-26 (user direction of 2026-11-26: each function of the old engine in the fast path
is removed, or improved and put into the new core; reviewed by Codex, source facts by Codex). For the main
agent. The user sends it; it is a user order. Copy this file with its SHA256 into
`R/night_20261115/FP3_A/inputs/`.
Code base: `a344c77269` ("FP2-P3: accepted; the closure is the default"), build673.
Short names: R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`; N = `R/night_20261115`;
P3 = the order FP2-P3 (your copy in `N/FP2_P3/inputs/`) and its record `N/FP2_P3/accepted_reference.json`;
"mode S" = both production options YES (`ADAP_NATIVE_LOCAL_CORE`, `ADAP_NATIVE_LOCAL_SURFACE`); "OFF" = both
NO; "CORE" = YES / NO; "worker stage" = one invocation of `AdaptMetric` inside an adaptation event (an event
can have more than one: a normalization correction); "the pass" = the initial construction-collapse
direction of a worker stage, from its seed census to its first `length_collapse_exit`; "step C" = the
surface collapse of the core. Section 1 describes the sealed P3 run, which had one worker stage for each
event.
Engine files are under `Common/src/adaptation/native_engine3d/`, core files under
`Common/src/adaptation/native_local/`.

Scope. One production change of the engine control, active ONLY in mode S when step C completed on all
ranks in this event: the construction-collapse queue of the engine stays closed on entry and on re-entry
to construction. The optional improvement cleanup stays available. In OFF, in CORE, with
wall heights, with a composition, and where the core is not eligible, expect the behaviour of build673; the
order reports identity for the data sets that it runs. One diagnostic environment variable with two probes
and one lab control value (section 3). The change alters the mesh in mode S: the acceptance is the rule of
the user (all standing gates pass; two runs with the same binary give the same files).

## 1. Why (measured on the accepted run S of P3, `N/FP2_P3/G5/build673/C7/trial2/full673_run1`)

Reviewer study: `reviewer_handoff/scripts/s3/study_engine_collapse_after_core.md` (read-only, on your logs).
- The pass takes about 143 s in each event (collapse `LENGTH elapsed` minus engine initialization and seed
  seconds): 23 % and 43 % of the remesh root. Each curved construction round has at most one active candidate
  for each rank (13,521 and 5,744 rounds). In the pass each rank has 83 to 129 s of elections and collectives.
- It accepts 1,576 and 1,286 collapses. By the net surface counts 690 / 507 of them remove a surface point;
  the logs give no class of each removal.
- The 28,062 / 30,495 construction trials include the later split trials. The logs show size admission as
  the main refusal category. They do not match those refusals to edges of the core.
- The worker runs the engine construction with `ValidityOnly` (no shape test); the core collapse has its own
  size and shape tests. The core reported 896 / 177 cells below its nodal-tensor Margin threshold at the
  hand-over. The engine reported 2,086 / 1,059 cells below its centroid-target q or J threshold at the
  quality entry. The two counts use different measures: the report compares them and does not say that the
  difference comes from the collapses.
- Step C protects feature-curve points and corners, runs before the volume work of each pass, and is not run
  after the last pass. The logs do not say which later engine removals step C had considered before.
- The final full-workflow size predicate limits long edges only. Short edges still affect the optional
  metric cleanup, and the separate core lab has its own lower-length gate.

| Assumption | State |
|---|---|
| With the flag false the behaviour is that of build673 | expected from the form of the change (default false; `Queue` unchanged). Step 1 reports identity for the data sets that it runs; the full OFF workflow is not evaluated |
| The modified mode S passes all standing gates of both events | NOT KNOWN. Measured by steps 2 and 3 |
| The event becomes faster by about 140 s | an estimate of the reviewer; measured by step 2 (report) |
| The quality step gets fewer bad cells and is not slower | NOT KNOWN. Measured by step 2 (report) |
| The split generations close the size gate from the seed state | NOT KNOWN. A standing gate of step 2 |
| What the optional metric cleanup does without the pass | NOT KNOWN. The suppression gate of step 2 allows a cleanup and counts it |
| Which engine collapses are useful, and which surface collapses the core lacks | NOT KNOWN. Measured by the probes of section 3. No gate |
| Two runs S give the same files | NOT KNOWN for this change. Gate of step 3 |

## 2. The production change

2.1 The trigger. Initialize the engine option of the event false (a new `bool` in `EngineOptions`,
`Common/include/adaptation/CNativeEngine3D.hpp`, default false; name it for what it says). Keep the existing
eligibility conditions of the core (`NativeRemeshRun.inc:699-712`). In the existing eligibility maximum the
rank values are: 2 for a missing sensor; else 1 if the surface option is requested and the local original
authority fails the existing eligibility test of `BeginLocalSurface`; else 0. A collective result below 2
admits the volume core. A result of 0 together with the surface option admits `BeginLocalSurface` and step C
on every worker. Keep the existing report of an unavailable surface where it applies. `RunLocalCore` today
returns whether it installed a new surface authority (it is false after zero surface edits): make it report
TWO things apart, the installation (used ONLY for the existing `caller_bytes` refresh) and "step C completed"
(true only on a normal return after ALL scheduled passes ran with step C enabled; zero candidates or zero
accepted edits still count as completed). Set the engine option from "step C completed". Keep the existing
failure propagation. The trigger reads no clock and is equal on all ranks.
2.2 The effect. `Selection` gets a flag `collapse_closed`, set from the option when the `Selection` is made
(`EngineAdapt.cpp:70-78`), before the seed frontier and the seed census. In `Selection::Queue`
(`EngineSelectionQueues.cpp:82`): return also when `coarsening && collapse_closed`. `Candidate` does not
change. `StartMetricCleanup` (`EngineControl.cpp`, before `LengthDirection(context, true)` at :1488) opens it
(`collapse_closed = false`), so that the optional strict-shape cleanup works as today.
`PreparePhaseDirection` (`EngineControl.cpp:573-580`, `next == Phase::Construction`) sets it again from the
option.
2.3 What must stay. After a complete seed census the initial construction collapse direction reaches its
ordinary exit without acquiring an operator round. Its length row, stage row, timer boundary
`length_collapse_exit`, the seeding of `generation_window[0]`, the ownership evaluation and the transition to
the split generations stay, with the same row structure. The seed census, the split generations, the
construction entry, the quality step, the final proof and the transfer are not changed.
2.4 No other production change except section 3. No config option. No new timer operation, no field in
`Session`, `CellState` or a row that travels (R10). `fixedstorage=4120` stays.

## 3. The diagnostic variable `SU2_NATIVE_FP3_PROBE` (read once per process)

No value or `0`: ordinary behaviour, no probe. `1`: the core probe (3.2) with the ordinary FP3-A behaviour.
`2`: an authorized LAB-ONLY control of mode S: it forces the engine option of 2.1 false (the pass runs as in
build673) and enables the engine probe (3.1). All probe measurement and printing also need `options.timings`.
No decision reads a probe counter. Export and verify the selected value on every rank after each launcher
cleanup. A run with `1` or `2` is an observation: it gives no acceptance and no reference.

3.1 Engine probe (value `2`). Count only the commits of the pass of the worker stage: start a fresh count at
each seed and print one row for each rank at the first collapse exit of that worker stage. The removed point is `choice.edge[1]`. Use its incidence in the
final old physical faces to say "surface removal"; then its binding before publication and
`MobilityDimension` give the dimension. The final cavity is "shared" when a removed cell has a dependency
owner other than the proposing rank. Compute the Margin of the old and of the fresh cells with the SAME
arithmetic as the local core (nodal tensors), from the imported `OwnedCell` nodal tensors and the transported
replacement `metric_rows`; match cells by id. Capture the observations locally before publication and add
them only when the existing accepted-slot accounting runs. One raw row for each rank and worker stage:
`NATIVE3D_FP3_ENGINE_COLLAPSE rank= accepted= removed_volume= removed_surface_2=
removed_surface_1= removed_surface_0= unclassified= shared= local= old_cells= old_bad= old_unmeasured=
fresh_cells= fresh_bad= fresh_unmeasured= status=`. `old_cells` and `fresh_cells` are ALL old and fresh cells
of the counted commits. `old_bad` and `fresh_bad` count the measured cells with
`Margin < SU2NativeLocal3D::BadMargin`, exactly as `bad_out` does. A measurement that fails or is not
available goes to `old_unmeasured` / `fresh_unmeasured`. No live metric or geometry query and no collective call for the probe. A fault of the probe leaves
the slot and the query result unchanged and prints `status=incomplete`.
3.2 Core probe (value `1`), a census of preparations. On the LAST live pass mesh, after its volume work and
before `WriteBack` and the final border shift, with the actual options, border marks and surface overlay of
that pass (checkpoint name `last_pass_pre_shift`). Enumerate each undirected edge of a local physical face
one time for each rank: take the edges as step C selects its seeds (the first physical-face use in the
`PhysicalFaces()` traversal, keyed by the sorted global end ids), then scan the keys in ascending order. Keep the edges with `Mesh::Length < 0.9`. Take the
ring and the physical parents as `CollapseSurface` does. Call the existing const `PrepareSurfaceCollapse` in
both directions, the removal of the lower id first. Count the edge in `prepare_pass` when one direction
has `.refusal == SurfaceCollapseRefusal::None`; else count the exact `SurfaceCollapseRefusal` value of the first direction (keep
`BorderKeeper` apart). `protected_same_curve` is evaluated for BOTH directed ends, independent of
`prepare_pass` and of the refusal bucket: count the undirected edge one time when in one direction the
removed end has curve mobility, the keeper is on the same canonical chain or is its end corner, and the edge
has an existing feature trace (resolve the chain from the stored bindings and the mobility policy; report a
missing classification as its own count). Print `proof=not_evaluated`: do NOT run contact, samples, entity, the final bound or a commit. One
row for each rank and worker stage: `NATIVE3D_FP3_HANDOVER_SURFACE rank= stage=
checkpoint=last_pass_pre_shift short_surface_edges= prepare_pass= <one field for each refusal value>=
protected_same_curve= unclassified= proof=not_evaluated status=`. Sums over ranks are sums of rank-local
edge incidences: say so in the report. Bounded local scratch, no collective call. A diagnostic fault that is
caught prints `status=incomplete` if it can and leaves the pass unchanged. When a watchdog or the end of a
process leaves a probe row missing or cut, the collector reports the observation as incomplete.
3.3 Stages. An adaptation event can call the core and the engine more than one time (a normalization
correction), with no new partition row; each call resets the engine counters. The raw engine row has
`rank=` only. The raw core row has `rank=` and the existing zero-based `stage=` of `RunLocalCore`. The
collector adds `event=1|2` to both (from the partition anchors) and a zero-based `stage=` to the engine row
(from the seed packet, step 2). Step 4 reports both tables by event, worker stage and rank.

## 4. The work, the gates and the labels

A new build number and receipt folders under `N/FP3_A/`. Build `SU2_CFD` and `UnitTests/test_driver`, no new
warning; freeze and pin ONE binary pair. One owned heavy job; the host preflight of R1 before each launch.
Runners (test code, R3): copy the P3 validators into FP3-A adapters with their own receipt roots and FP3-A
prerequisites. G2 / GQ: qualify ONE new capture with the default-mode admission of P3, then compare it with
the accepted P3 capture of that gate (comparator of N22 part C, scheduling calls exact). `saved_current`:
qualify ONE new run and compare the three payloads and the ordered planning projection with the accepted P3
case. CORE: the C6 settings and full-workflow validators for ONE run; its six hashes against the CORE entry
of the P3 record. S: launch and seal each trial by itself (the C7 adapter of P3 runs a pair: split it).
Steps 0 to 3 and their restarts run with the probe variable `0`: for each acceptance launch and restart,
verify and receipt the effective value `0` (and its MPI export where there are ranks) before the receipt is
sealed. G3 and C492 compare all `SU2_NATIVE_*` keys with a historical command: after the separate check of
`SU2_NATIVE_FP3_PROBE=0`, leave this ONE key out of that inherited comparison, in the normal path and in the
helper-timeout path; keep all other environment, config, target and input checks. Pin the order, the
sources, both binaries, the effective settings, the inputs, the gate receipts and both restart receipts.
Hidden unit cases (G1 does not change): the flag false gives the candidates of today on an existing engine
fixture; the flag true gives no initial collapse candidate, the ordinary exit, and a later metric cleanup can
queue again; step C completed with zero edits sets the trigger; the volume core running with step C not
available does not set it.

Step 0. One R4 source check of this text.

Step 1, the paths without the flag (closure default):
1. G1 identical to the reference (375 / 114 / 228); the hidden cases.
2. G2 and GQ against the accepted P3 captures: equal.
3. The shared checks: first collapse with its hard gates; exact C492; `merit481`; native 2D at 1, 2, 4 ranks;
   `saved_current` against the accepted P3 case.
4. G3 with the closure at its default: the standing physical gates AND, against the accepted P3 G3 case,
   identical accepted edits up to the smaller count and equal observer-adjusted entry work (in this order
   the identity is a gate again: bind the G3 adapter to the P3 case). G4: the P3 G4 evaluator on the same
   pinned native 2D executions of item 3.
5. One run CORE: all its full-workflow validators; the six files identical to the CORE entry of the P3
   record.
Label: "FP3-A: default paths unchanged", or "... FAILED", or "... OPEN". The full OFF workflow is "not
evaluated" in this order.

Step 2, mode S (after step 1 passed), the decisive run first: S run 1 with its isolated restart.
- Gates: all standing gates of both events (the inherited first-collapse findings of the S controller on
  work, count and refinement stay report-only, as in P3; the shared first-collapse check of step 1 is not
  changed), and the SUPPRESSION gate. Read each rank stdout in file order, without the launcher prefixes,
  and use only the rows whose `rank` is that of the file. The two `NATIVE3D_PARTITION` rows delimit the
  adaptation events. Inside an event each `NATIVE3D_SEED_SHORT` starts a worker stage and its
  `NATIVE3D_STAGE stage=exit` closes it. For each completed event read `Native3D frozen stages: count=N`
  from rank 0 (before the rank-field filter) and require exactly N complete seed-to-exit packets on every
  participating rank, with ordered `metric_cleanup_entry` / `metric_cleanup_exit` pairs. Inside each packet
  the initial collapse length / stage pair must come BEFORE any split exit, quality exit or final engine
  exit. Start the accounting again at each seed. For each worker stage:
  (i) at the initial `NATIVE3D_STAGE stage=length_collapse_exit`: `rounds=0`, `length_collapses=0`, and a
  paired `NATIVE3D_LENGTH pass=collapse` row with `collapse_accepted=0`;
  (ii) at each later construction-collapse exit: the same `rounds` as the `NATIVE3D_STAGE stage=quality_exit`
  that opened that construction phase again; pair the exit with the unpaired collapse length row before it on
  the same rank, and its `collapse_accepted` equals the `length_collapses` of the stage row;
  (iii) at each such exit and at the final engine stage: the cumulative `length_collapses` equals the sum of
  the increases over the completed cleanup pairs up to there.
  A cleanup is allowed: report its entries, exits and accepted counts. Report the stage rows apart when an
  event has a correction. The inherited G5 predicates on the final exit of an event stay as they are.
  When complete evidence shows that the engine went past a required construction-collapse boundary without
  taking it, that is a suppression RESULT failure (R5). When missing, cut or malformed evidence prevents
  the decision on what the engine did, it is a defective required measurement or a helper watchdog case (see
  "Failures").
- Report beside the P3 S pair (no gate): native maxima of both events; the time from the hand-over to the
  first `length_collapse_exit` and, apart, the pass time (collapse `LENGTH elapsed` minus engine
  initialization and seed seconds of the SAME worker stage: the `NATIVE3D_DETAIL stage=refine` row of an
  event holds the values of its final worker stage, so for an earlier stage of a corrected event give the
  raw collapse elapsed and write "pass time not available"); split generations and their seconds; the core `bad_out` and the engine
  `bad_union` at the quality entry (two measures, side by side); quality seconds; final cells, points,
  surface faces; short edges; job peak; timer coverage. Time flag: when the native maximum of an event is
  not at least 10 % below the faster P3 S run (the full-precision values of the P3 record), write "time OPEN
  for the user"; it is not a failure and stops nothing.
Step 3 (after S run 1 passed): S run 2, same binary pair, inputs and settings, with its restart. Gates: the
gates of step 2 for run 2, and the six files, hashed from disk, equal in the two runs. Then they are the new
S reference.
Step 4, observations (after step 3; they do not block the acceptance):
1. One run S with `SU2_NATIVE_FP3_PROBE=1`: the rows of 3.2; its six files against the new S pair (equal
   expected; report).
2. One run S with `SU2_NATIVE_FP3_PROBE=2`: the rows of 3.1; its six files against the S entry of the P3
   record (equal expected; report).
Both use the existing physical and resource limits. Report both probe tables by event, worker stage and rank,
together with the probe status.

Acceptance (when steps 1 to 3 passed): publish `N/FP3_A/accepted_reference.json` with a `workflows.S` entry
in the form of the P3 record (config; both case paths, full-run and restart receipts with SHA256; the six
hashes; the native maxima; the pins) and append to `AGENTS.md` section 2a: "- Since FP3-A the S reference is
the `workflows.S` entry of `R/night_20261115/FP3_A/accepted_reference.json` (in mode S the engine makes no
construction collapse after step C completed; the optional improvement cleanup stays). Ordinary S runs use
it. The FP2-P3 record stays in
force for OFF, CORE, G2, GQ, G3 and `saved_current`, and its S entry stays as the reference of the old
behaviour (`SU2_NATIVE_FP3_PROBE=2`, a lab control)." Update the progress file, `HANDOFF_Codex.md`,
`HowToProceed.md`, `Handbook/ADAPTATION_TIMINGS.md`. Label: "FP3-A: accepted; no engine construction collapse
in mode S". Report `N/FP3_A/reports/FP3_A_<build>.md` and `.json` (status of each check; the tables of step 2;
the probe tables; "not evaluated": the full OFF workflow; no recommendation). Then STOP.

Failures. R5 applies to each required result gate of steps 1 to 3, the suppression gate and the six-file
pair included: keep the first difference and the evidence; one diagnosis of at most 60 minutes; one wiring
correction and one repeat. If the result failure stays or its cause is in this text: revert the connected
production change of FP3-A in ONE commit, the lab control included; label "FP3-A: FAILED; the engine pass
stays"; report; STOP. The part-B exception of P3 does not apply here. A required measurement that is still
defective after the R3 / R6 work is FAILED as a measurement: it prevents the acceptance and needs no
production revert. A helper watchdog: the retry rule of P3 section 6 (one retry at three times, the listed
limits stay gates); a second helper-only end is OPEN. A check that a prerequisite blocks is "not evaluated";
finish the independent checks (R12). A changed binary pair needs all required acceptance evidence again on
the new pair. Label precedence: FAILED, then OPEN, then accepted. A failure of a probe only: R3 / R6, reported
apart; it does not prevent the publication after steps 1 to 3 passed.
The rules R1, R3, R5, R6, R7, R8, R9, R10, R11 (the probe variable with `options.timings` is the written
device), R12 and R13 hold. Expected machine time: about 3 hours.

## 5. Not in this order

A change of the core (step C after the volume work, feature-curve points: later items, designed from the
probe rows). A change of the split generations, of the quality step or of the final proof. A change in OFF,
in CORE or for wall heights. A config option. A new reference for OFF, CORE, G2, GQ, G3 or `saved_current`.
The full OFF workflow.
