# FP2-M1: measurement of the support of the geometry bound query (lab test code only)

Author: reviewer, 2026-11-24 (source anatomy and review by Codex). For the main agent. The user sends it; it is a
user order. Copy this file with its SHA256 into `R/night_20261115/FP2_M1/inputs/`.
Code base: `af5844543d` ("FP2-C3: S passed; default path not yet proven"), build664. The kept candidate does not
change. This order changes TEST CODE ONLY. No production source file changes (R2): no file under `Common/` and
`SU2_CFD/` is edited. If a step of this text seems to need a production edit: do not do it, write it in the
report (R4) and do the rest.
Short names: BP = `Common/src/adaptation/CNativeBoundaryParameters3D.cpp`; AR = `Common/src/adaptation/
CNativeAffineReference.cpp`; ARh = `Common/include/adaptation/CNativeAffineReference.hpp`; SG = `Common/src/
adaptation/native_local/CNativeLocalSurfaceGeometry3D.cpp`; OBS = `UnitTests/SU2_CFD/adaptation/fixtures/
native_c3_lab_observers.inc`; VH = `native_volume_handover.inc` (the lab driver with `LabBoundObserver`).
"The lab" = the lab of FP2-C3 continuation 1 (build664) with its runner, comparator and one-trial evaluator.

## 1. Why

Your run S of build664 passed its gates, but the native maxima are 951.321 / 689.037 s (build660: 550.220 /
330.735 s). The sum of `exclusive_seconds` of the `NATIVE3D_TIMER root=remesh boundary=final` rows over the
phases, for each rank (ranges over the ranks, event 1, build660 -> build664), shows one cause:
- `geometry_bound`: 80-188 s -> 197-450 s with FEWER calls (166k-286k -> 148k-277k); the mean cost of a call is
  0.4-1.0 ms -> 0.9-3.0 ms. Event 2: 35-130 s -> 72-392 s.
- `communication` + `join_wait`: 211-308 s -> 369-613 s. All other rows together: 140-204 s -> 120-192 s.
- Your table 3.1 (N1): the mean cost of a call grows with the longest edge of the face (1.81 ms below 2 mm,
  22.96 ms at 20 mm and more); 772.852 of 775.167 s are on the non-direct path; the counter of N1 shows one
  step-C bound refusal in 72,564 forwarded queries.
- The hand-over has 26,480 / 21,308 physical faces (build660: 57,640 / 51,808).
So the cost of one query on a longer face is the item, in the core and in the engine.

The non-direct path of `BoundaryParameterState::EntityFaceError` (BP:526-539) takes the box of the three points,
makes it larger by `radius` on each side (`radius` = the largest distance of a point to `Realize(binding[0])`,
plus the tolerance: about the size of the face), takes the reference facets of the entity whose boxes touch it
(`NearbySurfaceFacets`), and gives them as the support to `BoundPhysicalSurface(..., Classification)`.
The full box is intended to include the nearest candidates in the indexed entity component
(`Handbook/ADAPTATION_FACE_ADMISSION_DECISION_489_20261113.md:69-75`). A refusal of the classifier can also mean
that the upper proof did not resolve. An upper bound at or below the tolerance from a SUBSET of the support
proves the one-sided claim (the face is near the model). This order measures how often a small support gives
that proof, and at what cost. It designs no rule.

| Assumption | State |
|---|---|
| A result `<= tolerance` of `BoundPhysicalSurface` with a SUBSET of the support is a proof for the face (one-sided: the face is near the model) | from the source (ARh:209-215; AR:303-312, :2503-2533; read by Codex). Check it in your R4 source check |
| The returned upper bound is monotone in the support | NOT TRUE (AR:296, :379, :577: seed order). A small support can prove a face where the full one does not, and the reverse |
| A small support proves nearly each face that the full support proves | NOT KNOWN. Measured here |
| The query with a small support is much cheaper (the upper witness is already cropped to pieces near the face, AR:284-312; the support-wide loops are not) | NOT KNOWN. Measured here |
| An extra query on the SAME authority can change the answer or the cost of a later production query | TRUE in the source (`BuildPlanes` keeps a footprint that depends on the storage headroom of the first caller; memo and pool eviction; AR:492-568, :1106-1131, :1211-1249). So the small queries run on a SECOND authority (2.1) |
| The engine can use the results of the core for the faces that survive | NOT KNOWN. One row here (2.6) |

## 2. The measurement (test code; a new lab option, off in each other test)

With the option off the lab works as in build664. With the option on:

2.1 The shadow authority. Each rank opens ONE second, independent authority for this measurement. The lab runner
supplies the same pinned `.native_ref` path, resolved from the mesh file name as `LoadMesh` does
(`CNativeLocalCoreLab_tests.cpp:121-128`; the path is local there). Call the budgeted `LoadReference` overload
(`CNativeReferenceIO3D.cpp:202-204, :284-292`) collectively, before the first pass, outside the hook of a face.
Do NOT copy the `MappedFacetReference` of the pass and do not use
`Authority()` of the state: a copy shares the caches and the statistics. The immutable geometry definition of the
shadow must be equal to that of the authority of the pass before a shadow result is collected (else: no
measurement for the case, an R3 / R4 item). Report a difference of `memo_limit` or `prepared_limit` (the loader
derives them partly from the free host memory) beside the replay estimates.
Which object each call uses:
- the SHADOW: `Canonical`, `Supports`, `Realize`, every `NearbySurfaceFacets` and `BoundPhysicalSurface` call
  of this measurement, and the statistics snapshots of the measurement;
- the concrete surface of the pass (`Find`): the live bindings and the coordinates (also of points that the
  pass made);
- the state of the pass: the tolerance (`GeometryTolerance()`) and the mobility policy. Form the entity tuple
  of a facet with the body and the marker of the SHADOW and the sheet of the policy of the pass (the canonical
  id of the shadow when the policy has no identity), as `Entity` (BP:430-432) does;
- the authority of the pass: only the forwarded production call, the strict hand-over calls, and read-only
  statistics snapshots for the rows of 2.6 and 2.7.
Storage. Charge the load peak of the shadow, its resident bytes and the scratch of the observer to a SEPARATE
observer ledger, and report them. The caller bytes of the pass, the contact admission, the geometry budget and
the modeled-memory gate of the pass stay exactly as in build664 (the admission of build664 left about 10 kB
below the modeled ceiling: the shadow must not go into it). All allocations of the observer are inside the
actual job-memory gate of the lab, which does not change. Choose the shadow mode collectively (all ranks) before
the first pass. If the shadow cannot be loaded inside the observer ledger and the job-memory limit on a case:
the isolated measurement of that case is OPEN (write the numbers; go on). A measurement on the authority of the
pass is only an optional diagnostic: each row of it carries `interleaved=1`, and it never counts as the
measurement of this order. Keep the campaign trace off, and keep the observer outside active production timing
and D9 sessions (the trace flag and these contexts are shared by all authorities of a process).

2.2 The budget. Build an own observer ledger with the existing fixture helper `Budget`
(`native_fast_path_stage2_setup.hpp:15-36`): the same ceiling, integer-bit limit and work-limit formula as the
pass. Each query of this measurement gets a fresh `Budget` with the start of the trial and its 5,400 s limit.
The ledger of the pass does not change. Charge the support vectors that the observer keeps, and report a
conservative bound for the scratch of the candidate selection (`NearbySurfaceFacets` does not charge its own
list). Outcome `resource`: work or time exhaustion (they are sticky), and each documented storage, integer-bit
and allocation refusal (some arrive as `runtime_error` from the exact-arithmetic helpers), also when it happens
during the selection. Each other exception: `threw`. In every case the forwarded call gives its answer or its
exception as without the observer.

2.3 The inputs. In `LabBoundObserver::WithinTolerance` (VH), after the capture of the sweep stamp and BEFORE
`BoundWorkBefore()`: take the canonical facet; rotate `face.v` cyclically so that its smallest id is first (keep
the orientation), as SG:758-776 with `FaceOrder`; read each binding and `node.p` with `Find(id)`.
If `Supports(facet, binding)` holds for all three corners: the call is `direct_eligible`; no small query.
(Such a call can still run a bound with one facet in production, BP:513-525. So the name says only which branch
it takes.)
Else, with the formula of BP:526-535 (three-argument `hypot` to `Realize(bindings[0])`; `nextafter` on `r` and
on each box end, outward, exactly as there):
- `S_full` = `NearbySurfaceFacets(facet, B(r), &ledger)` with the facets of another entity tuple removed
  (BP:535-536). `B(pad)` = the box of the three points made larger by `pad`. `S_full` is used for the equality
  labels, the nesting check and the support sizes; time its selection apart ("diagnostic selection") from the
  selection of the small sets. If its selection fails: stage-1 outcome `resource` or `threw` with
  `where=select_full`, no stage 2 for the call, forward as usual, and keep the call out of the replay estimates
  (report its count).
- `S_1` = the same with `pad1 = min(r, nextafter(2 * tol, +inf))`.
- `S_2` = the same with `pad2 = r / 4`, only when 2.4 needs it and only when `pad2 > pad1`.
The sets are nested (`S_1` in `S_2` in `S_full`): assert it (a failure is an R3 fault of your code or an R4
finding).
Take the work snapshot of the production call and start its clock AFTER all queries of 2.3 / 2.4, so that the
existing table 3.1 keeps its meaning.

2.4 The small queries (on the shadow, with a ledger of 2.2). Each call that is not `direct_eligible` gets
exactly one outcome for each stage that it reaches. Stage 1:
- `empty` (no facet); `same` (`S_1` is equal to `S_full`; no query);
- else one `BoundPhysicalSurface(S_1, {points}, tol, ledger, Classification)`: `proved` (result `<= tol`),
  `fail`, `resource`, or `threw` (count each exception text, at most 5 texts).
Stage 2, only when stage 1 is not `proved`; test in this order: `no_larger_box` (`pad2 <= pad1`); `same_as_1`
(`S_2` equal to `S_1`); `same_as_full`; else one query with `S_2`, outcomes as stage 1.
A `fail` or an exception means "not proven by the small support", not "outside".
For each query record: the seconds; the difference of the work of its ledger; the difference of
`classification_work` and, where `GeometryStatsSnapshot(false)` of the shadow has them, of the selected and
total chord pieces; the support size. Then forward the call as today.

2.5 The check of your copy of the inputs (in the focused fixture ONLY; not in the campaign).
- Start the fixture process with the environment variable `SU2_NATIVE_GEOMETRY_WORK_TRACE=1` (it is read one
  time, at the first use; no compile option). Put a lab marker with the ordinal of the call around each checked
  forward that is not `direct_eligible`. The entry row `NATIVE_SURFACE_QUERY authority=... support=...
  triangles=...` of the forward (the one before the memo lookup; not the second format inside
  `QuerySurfaceUnion`) must have exactly your ordered support ids, your tolerance bits and your ordered triangle
  bits. Gate of the test code: at least 20 matched forwards, each identical.
- The trace does not print the vertex ids, the first binding, the anchor or the radius. Assert these apart
  against values that the fixture knows, with at least one case where the wrong anchor would select another
  support.
- A mismatch is an R3 fault of your code.
- Also in the fixture only: after the forward, one full-support query (`S_full`) on the shadow with a fresh
  ledger. Report `replica_attempted`, `agree`, `disagree` (the result compared with `tol` differs from the
  production answer), `resource`, `threw`. A disagreement with a passed input gate is a report, not a failure.
In N6P and N1 the shadow does the small-support work only (a full query there would warm the caches that the
later small queries use).

2.6 Reuse at the hand-over. At the existing strict check (`ObserveLabStrictFaces`, VH; it uses the surface and
the authority of the pass): for each rank record the calls, the seconds, the difference of
`classification_work`, the differences of the memo counters of `GeometryStatsSnapshot(false)`, and the count of
calls with a zero difference of `classification_work` as its own number (a direct call also has zero). Row
`FP2_M1 HANDOVER rank= calls= seconds= work= memo_union_hits= memo_direct_hits= memo_misses= memo_evictions=
memo_retention_refusals= zero_classification_work_calls=`. The sum of `calls` over the ranks must be equal to
the sum of the `strict_observation checked` values of the ranks and to `strict_checked` of the global proof. Do
not name step C as the source of a hit: `ReportCollapseFaces` also asks before the hand-over.

2.7 Rows (print with the existing off-stream method). `pass=` is ONLY the pass id; the count of proofs is
`proved=`. For each rank and pass:
- `FP2_M1 SUPPORT rank= pass= stage=1|2 interleaved=0|1 calls= direct_eligible= empty= same= no_larger_box=
  same_as_1= same_as_full= proved= fail= resource= threw= k_full_sum= k_small_sum= seconds_select_small=
  seconds_select_full= seconds_small= work_small= classification_work_small= full_yes_small_proved=
  full_yes_small_other= full_no_small_proved= full_no_small_other=`. `calls` of stage 1 = all forwarded calls
  (the direct-eligible ones included, in their own count); of stage 2 = the calls that came to stage 2.
  `full_yes` / `full_no` = the answer of the forwarded production call. The four agreement counts and the
  `DIFF` rows are without the direct-eligible calls.
- `FP2_M1 FALLBACK rank= pass= group=proved_1|proved_2|needs_full|direct_eligible calls= seconds_full=
  work_full= full_memo_hits=`: the seconds and the classification work of the FORWARDED production query,
  summed for the calls that stage 1 proved, that stage 2 proved, that no small query proved (with `same`,
  `empty` and the other outcomes), and the direct-eligible calls. `full_memo_hits` = the forwarded calls that
  were memo hits, if the counters show it; else `not_available`.
- `FP2_M1 SUPPORT_BIN rank= pass= kind=length|fate bin= calls= proved_1= proved_2= needs_full= seconds_small=
  seconds_full= seconds_full_needs_full= k_full_sum= k_small_sum=` with the bins and fate groups of table 3.1.
- `FP2_M1 DIFF rank= pass= stage= case=full_no_small_proved|full_yes_small_other ...`: only for calls that
  occur: each `full_no_small_proved` call (at most 200) and the first 50 `full_yes_small_other` calls, with the
  fields of your `BOUND_SLOW` row plus `k_full= k_small= result_small= outcome= r= pad= r_over_tol=`. Print the
  totals of both cases also when they are 0.
- `FP2_M1 OBSERVER rank= pass= status= seconds_observer= scratch_bytes= shadow_bytes= shadow_load_peak=
  ledger_peak=`; `FP2_M1 MEMO rank= pass= at=start|end authority=pass|shadow hits= misses= entries= evictions=
  retention_refusals= bytes=`. In the fixture also `FP2_M1 REPLICA attempted= agree= disagree= resource= threw=`.
A failed observer prints `status=failed` and no table; the forwarded production answer and its exceptions never
change. A trial with a missing or failed table of this order is not a completed measurement.

## 3. The runs

A new build number and new receipt folders under `R/night_20261115/FP2_M1/`. Build `UnitTests/test_driver` only
(`FP1/build_fp1.py` has this mode), no new warning. The receipt shows: the production source pins and the hash
of the `SU2_CFD` binary are those of build664; the new test binary; the changed `.inc` files and the helpers of
this order are pinned.
1. The focused fixtures of FP2-C3 continuation 1 (9 serial, 3 two-rank) with the option off: all pass, no pinned
   value changes. G1, the FP1 cohorts and the run S are NOT run in this order (no production change).
2. One focused curved fixture with the option on (an existing curved lab fixture or the smallest lab case):
   observer status ok; the gate of 2.5; `proved > 0` at stage 1; `replica_attempted > 0`; the rows `SUPPORT`,
   `FALLBACK`, `REPLICA`, `OBSERVER`, `MEMO` are there. `DIFF` rows and stage-2 queries need not occur.
   The fixture must have an indexed curved component of MORE than 16 facets (a smaller component has no search
   index: each box gives the same support, AR:1328, :1952) and a face that is not direct-eligible for which
   `S_1` is a proper subset, not empty, that proves the face.
3. The lab with the option on, ONE trial each: N6P, then N1. The host preflight of R1 before each launch; keep
   the sampling of the supervisor light in the six-rank case.
   - Runner. Own FP2_M1 runner glue around `evaluate` of `FP2_C3/run_c2_lab.py` (the campaign driver of FP2-C3
     starts six trials and asks for G1 / FP1 receipts; do not use it). Keep the outer checks of one trial: exit
     status, Catch, rank logs, terminal owned processes, job memory; and all gates of `evaluate` (the last-pass
     `out` / `lt_1e_2` zero condition included). Teach the row parser the `FP2_M1` rows.
   - Watchdog. The wrapper timeout is 3 times the sealed `wait.elapsed_seconds` of trial 1 of build664 of the
     case, rounded up to a whole second: N6P 2,575 s; N1 3,261 s. The same value for the first attempt and for
     the one retry; the receipt names the retry (not a changed timeout). Retry only a wrapper-only timeout,
     after all owned processes are terminal. A result, modeled-memory, job-memory or in-process deadline
     failure that was seen wins over the watchdog. A second wrapper-only timeout is OPEN (write it; go on).
   - Identity. After a trial qualified alone: verify trial 1 of build664 of the case against the SHA256 in
     `lab_acceptance.json` and verify its artifact manifest; then run `repeat_differences`
     (`FP2_C3/compare_lab.py`) between the `rows_by_rank` of the new trial and of that trial 1
     (`FP2_C3/cases/build664/{N6P,N1}/measurement.json`). The complete repeat projection must be equal, the
     `modeled_peak` of the pass included. The storage of the observer is reported apart.
   - A difference in the identity or a failed inherited result gate. First R5: if the cause is a transcription
     or wiring fault of your test code, correct it one time and repeat the trial one time. Else it is a FINDING
     (the measurement touched production behaviour). Preserve the evidence; write the first row, field, rank
     and pass. Do NOT change production code. Keep the test code. The case gets "FAILED" in the report. N1 is
     independent of N6P: run it also after an N6P failure.
   The numbers of section 2 are a REPORT. They have no gate.

## 4. The report

`R/night_20261115/FP2_M1/reports/FP2_M1_<build>.md` and `.json`. For N6P and N1, for each pass (the sum over
the ranks, and the rank with the largest forwarded `seconds_full`):
- the outcomes of stage 1 and stage 2 with the four agreement counts; `interleaved`;
- the cost of the small path: `seconds_select_small` + `seconds_small` of stage 1 and of stage 2 (each apart);
  the diagnostic selection of `S_full` apart;
- the measured cost of the full query of the SAME run for the groups `proved_1`, `proved_2`, `needs_full`,
  `direct_eligible` (row `FALLBACK`), and the same total in build664 (your table 3.1);
- two replay estimates of the whole stream of calls: (a) the timed small path of stage 1 plus the forwarded
  seconds of the calls that such a policy still forwards; (b) the same with stage 2 between them. Both include
  the direct-eligible forwards. Write beside them: both estimates use the cache history of this run (an earlier
  stage-2 query can change the cost of a later stage-1 query; each production forward can change the cost of
  later forwards); and the skips `same` / `same_as_full` use information that a real small-first path must
  compute or do without;
- the tables by longest edge and by fate; mean `k_full`, mean `k_small`, mean `r / tol`; the chord-piece
  counts where there are any;
- each `DIFF` row of the case `full_no_small_proved`; the count of the other case (its rows in the json);
- the row of 2.6 for each rank beside `strict_checked`; the `MEMO` rows; the `REPLICA` row of the fixture; the
  seconds and bytes of the observer and of the shadow (resident, load peak, RSS apart); the trial seconds and
  the job peak beside build664.

## 5. The order of the work and the labels

0. One R4 source check of this text (the source statements of the table of section 1 included).
1. The test code and the runs 3.1 and 3.2. Commit and push: "FP2-M1: observer fixtures passed; default path not
   yet proven".
2. The two lab trials and the report. Commit and push: "FP2-M1: measurement done; default path not yet proven".
   If a case failed (a finding of section 3, item 3) or has an incomplete measurement that is not explained:
   "FP2-M1: measurement FAILED; default path not yet proven" with the evidence. If the only incomplete cases
   are recorded as OPEN: "FP2-M1: measurement OPEN; default path not yet proven". Then STOP and wait for the
   user. Update the progress file, `HANDOFF_Codex.md`, `HowToProceed.md` and `Handbook/ADAPTATION_TIMINGS.md`
   (the numbers of section 1 and of the report).
The rules R1, R3, R5, R6, R7, R8 and R13 hold as in each item.

## 6. Not in this order

A change of `EntityFaceError`, of the support box, of test 11, of the memo or of an engine query. A rule from
the numbers: the reviewer designs it after the report. The engine callers (their needs `Magnitude` and `Limit`,
their pass rates) are not measured here; that needs a later print-only production counter. The lab case N6, a
second trial, the FP1 cohorts, G1 and the run S.
