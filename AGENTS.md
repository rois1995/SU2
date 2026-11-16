User order, 2026-11-16 (LATEST): NIGHT_WORK_ORDER.md section6.8; wins over I part A where different. Do not stop a running job.

### 6.8 Correction of 2026-11-16, 17:00: N8 A1 again, in the group of A2 / A3 / N12 (two reviewer errors; this section wins over I part A where they differ)

Facts, from your evidence (`R/night_20261115/N8/A1/REPORT.md`, `R5_diagnosis.json`,
`source_check.md` risks 7 and 8):

- Error 1 of the reviewer. The check "the same edge CSV hashes" cannot hold. The law
  moves from a test file (compiled with fast-math) into the list of the native
  adaptation sources (`accurate_sum_lib`, strict arithmetic). That place is correct:
  a law that a decision reads must have strict arithmetic. So the last digits of the
  scores change (49,392 scores, largest relative difference 3.96e-8), and the
  limiting side changes where two sides have the same score inside that difference
  (298 / 296 rows). The rows, the `need` flag of each of the 82,559 edges and all other
  columns are equal. Your code did what the text asks. Your revert followed the written
  check; it was correct to do it.
- Error 2 of the reviewer. I A1 item 5 asks a new timer operation `wall_law`. A new
  `Operation` makes `sizeof(Session)` larger (192 B). `Session::ReservedBytes()` goes
  into `caller_bytes` (`CNativeEngine3D.cpp` 1355, `CNativeRemesher3D.cpp` 212), so each
  modeled byte row and the storage admission would change. This is not "identical
  results". G2 would have failed for this cause.

1. When. Do not stop a running job. Let the check that runs now end. Then, before the
   next check of the group A2 / A3 / N12, put A1 into the same group (items 2 to 4).
   The checks of the group that passed on build 533 run again on the new build; the
   join still saves one full set of gates and one full run.
   Exception: if the full two-event run of the group is live or done when you read
   this, finish and judge the group as written. Then do A1 alone with items 2 to 5.

2. Put back the production and test paths of `eb6d94042c` on the current head, with ONE
   change:
   - no `Operation::WallLaw` and no `wall_law` name. `PrintWallLawCensus` opens
     `Scope(Operation::Census)` (the operation that `observe_progress` uses already).
     `sizeof(Session)` must not change: each `NATIVE3D_TIMER_SUMMARY` row prints
     `fixedstorage=4120`, as the reference does.
   - Consequence for your comparators, an exact rule and not a tolerance: for each rank
     and each timer boundary, the sum of `calls` over the rows of the operation
     `census` is (reference value) + (count of `NATIVE3D_WALL_LAW` lines that this rank
     printed before that boundary). Each other old row stays strict: integers, modeled
     bytes, calls.

3. The check of A1 on the three N5 geometry fixtures (replaces "the same edge CSV
   hashes" of I "Checks of A1"). Against the R1-repeat receipt of each fixture:
   (a) the same rows in the same order (`edge_a`, `edge_b`);
   (b) in each row, identical: `tau`, `need_threshold`, `need`, `supported_sides_a`,
       `supported_sides_b`, `unsupported_sides_a`, `unsupported_sides_b`;
   (c) in each row: |`score_max` - reference| <= 1e-6 x max(reference,
       `need_threshold`);
   (d) the same M1 integer rows (7,521 / 6,759 / 4,368 need edges);
   (e) report only: the count of rows with another `limiting_side_a` or
       `limiting_side_b`, and the largest relative score difference.
   The edge CSV files of this run are the law reference from now (strict arithmetic).

4. One more print-only change in the same build (for the reviewer's text on the wall
   refusals). The N9 table has 345 times one text, 15 texts that differ only by a facet
   and a cell number, and 544 in `more`. So: before the lookup in the 16-entry table,
   replace each run of decimal digits in the stored 96 characters by one `#`. Nothing
   else changes. The sum of the counts must be the count of operator refusals.
   And, print only, in `CNativeWallDiagnostics3D.hpp` (`ObserveGuard`): the class
   `other_geometry` has three more classes beside it, by the operation name that
   `guarded_geometry` gets already: `RefitWallSplit`, `ReconstructWallSplit`,
   `ReconstructSplitPatch`. For each of these three: the count of calls, the count of
   refusals for each reason (the `NATIVE3D_WALL_REFUSAL` rows that exist), and the
   seconds of the call (one clock around the call in the frame loop, only when the
   counters are enabled). New line at the stage exit:
   `NATIVE3D_WALL_FRAME rank= operation= calls= valid= seconds=`.
   The refusals of the three new classes and of the rest of `other_geometry` must sum
   to the old `other_geometry` rows. No decision reads these counters.
   Reason (reviewer's source reading, for the report): `SplitEdge` and
   `InsertEdgeCavity` have 0 refusals in the N9 run. All 904 refusals come from these
   three operators. The engine calls them only after the first split of a choice of
   the geometry queue was not approved. So they follow the guidance rejections.

5. Gates of the group (A1 + A2 + A3 + N12 + item 4), one time: build; the unit cases
   of the items; item 3; M6b; G1 by identity; G2 and GQ with all old integer rows equal
   (rule of item 2 for the `census` calls); G3 by rule 1 of 6.6; G4; the safeguards;
   ONE full two-event run with its six files equal to the references. Report the
   `NATIVE3D_WALL_LAW` rows with their seconds, and the new reason table of the G3 run.

6. If a result gate of the group fails: R5. The one diagnosis pass names the part from
   the first number that differs (A1, item 4, or A2 / A3 / N12). Revert that part only
   in one commit, build, repeat the failed check one time, and go on with the group.
   If the pass cannot name the part: revert A1 and item 4, and go on.

7. After the group has passed: N11 (B2), then N10, as items 4 and 5 of 6.6 say. Then
   6.3, N6, N7.


User order, 2026-11-16 (LATEST addition): section6.7, N12/M24 print-only with N8 A2/A3; do not implement8.5.

### 6.7 One more print-only measurement: M24, the lane census (2026-11-16, 15:00)

Purpose: design step 8.5 (not accepted, do not implement it). The reviewer needs one
number to write its text: which share of the quality work a rank could do alone.
Text: D section 5.11 and row M24 of D section 9 (D revision 2.2; copy it with its
SHA256).

Item N12. Do it in ONE build with N8 parts A2 and A3 (all three are print only), with
the same gates and the same full two-event run. No extra run.

1. Where: the private loop of a round, for each slot that reaches `evaluate_candidate`,
   in the quality stage and in the construction stage. Under `options.timings` only.
2. Class of the slot, from data that the slot has already:
   - `local` = each `dependencies.cells[i].owner` is this rank;
   - `wall` = one or more `imported.cells[i].physical` facets are bound.
   Four classes: local / shared x interior / wall.
3. For each class and stage: trials, approved patches, commits, busy seconds of the
   slot (the clock that the private loop has), optimizer work.
4. For the commits of each class: the count with `SameSkin(old, fresh)` true, the count
   with no parameter edit, the count with an empty coverage delta. Compute `SameSkin`
   for this only where the code does not compute it already; its result must not be
   read by a decision.
5. Print at the stage exit, one line for each rank, stage and class:
   `NATIVE3D_LANE_CENSUS rank= stage= class= trials= approved= commits= seconds= work= same_skin= no_parameter_edits= no_coverage=`.
   The sums over the classes must agree with the ladder counters that exist.
6. Gates: identical results. G1, G2 and GQ with all old integer rows equal, G3 by rule
   1 of 6.6, G4, and the full run of N8 with its six files equal to the references.
7. Report: one table for each event (6 ranks x 2 stages x 4 classes), and the share of
   the busy seconds and of the commits in the class local + interior.


User order, 2026-11-16 (LATEST): NIGHT_WORK_ORDER.md section6.6, wins over6.5/I/C where different. Do not stop a running job.

### 6.6 Correction of 2026-11-16, 14:50 (reviewer errors; this section wins over 6.5, over I and over C where they differ)

Facts, from your evidence (`R/night_20261115/N9/R5_diagnosis.json`, `REPORT.md`,
`B0_table.md`):

- The G3 fixture has two clock inputs. The observer before `PrepareSurface` runs for
  30 s and copies its work into the shared atlas budget
  (`UnitTests/SU2_CFD/adaptation/fixtures/native_actual_surface_target_observation.inc:18`),
  and the stage has 388.8 s. Three runs with identical decisions gave 473, 486 and 459
  splits. So the gate "all old integer rows of G3 equal" cannot hold. This gate text was
  an error of the reviewer. N9 did what its text asks: 459 common edits identical, entry
  work minus observer work 5,113 in both runs, counter sums agree. Your revert followed
  the written gate; it was correct to do it.
- N9 table: the guidance check rejected 1,180 of 1,437 trials (82 %) for choices of the
  geometry queue and 10 of 217 (5 %) for choices of the size queue. Cause `incomplete`
  1,187 times: `GeometryGuidanceExcess` stops at the limit `before`, that is, the new
  wall faces have more summed excess than the old ones. Cause `no_progress` 3 times.
- 865 of the 912 operator refusals have the label `other_geometry` and the class
  `other`. So their cause is still not known.
- 3,508 of 4,737 selected choices are `deferred` before a trial.

1. Gate G3, for all items and for C, from now:
   (a) Physical part, as before: all private cells positive with sensors, zero upper
       refinements, the modeled storage and the job peak inside their limits, at least
       336 splits, the first 336 edits identical to the P6c reference.
   (b) Identity part, for an item with "identical results": the accepted edits (edge and
       point of each edit) are identical to those of the G3 reference run, up to the
       smaller accepted count of the two runs; and `entry atlas_work` minus the observer
       work is equal in the two runs.
   (c) All other integer rows of G3 (entry `atlas_work`, rounds, split count, exit
       counters) are a report beside the reference. They are not a gate.
   The G3 reference run is the C492 run of section 6.1 (486 splits). G1, G2, GQ, G4 and
   G5 do not change. If you find a clock input in one of them, report it; do not change
   the gate.

2. N9 again, before the next production edit of another item.
   - Put back the three production paths of `bd709cb4c1` on the newest accepted commit.
   - One addition, print only: for each guarded trial that an OPERATOR refuses (origin
     not `Guidance`), count the distinct texts of `result.reason`. A fixed array of 16
     entries (the first 96 characters of the text, and a count) and one count `more`.
     Print at the stage exit: `NATIVE3D_WALL_REASON text="..." count=`. No decision reads
     it. The sum of the counts must be the count of operator refusals (912 in the table).
   - Gates: build; G3 by rule 1 (one run); G2. Report the B0 table again with the new
     rows. Then N9 is PASSED.

3. New order: N9 (item 2) -> N8 part A1 only (the law) -> N11 (B2) -> N10 (B1, with the
   new text of item 5) -> N8 parts A2 and A3 -> 6.3 -> N6 -> N7.
   - The G1 reference that you record now on the accepted base stays the G1 reference
     for these items.
   - If a production edit of B1 with the OLD text exists when you read this: do not
     judge it. Let a live job end, keep its numbers as "B1, old text, not judged", put
     the production paths back to the accepted base in one commit, and follow this order.
   Reason of the order: the 82 % against 5 % (facts above). Reading of the reviewer (I,
   N11 tests it): the geometry queue gives the score of a failing face to its edges by
   their length. So it splits the longest edge, also when the curvature is along another
   edge. The two new faces then keep the error, the summed excess is twice as large, and
   the guidance check rejects the split. For such a split the check is right. B1 with
   the old text would accept these splits. So B2 comes first, with the check in place.

4. N11 (B2): the text of I part B2 does not change. Its gates: I "Gates of B1 and B2"
   items 1, 2, 4 and 5 as written; item 3 is replaced by:
   - the G3 fixture two times; validity as in rule 1 (a); the two runs have the same
     edits up to the smaller of their accepted counts;
   - purpose: in BOTH runs, for the choices of the geometry queue, (rejections of the
     guidance check) / (guidance trials) is below 0.70. The reference is 0.82
     (1,180 / 1,437; two older runs differ by 0.003 for all queues together). If it is
     not below 0.70: the reading of item 3 is wrong. Keep the numbers, revert B2 in one
     commit (R5, cause in the proposal), do NOT do N10, and go on with N8 A2 / A3;
   - report beside the N9 table: all B0 rows, `law_weighted_faces`,
     `law_fallback_faces`, accepted splits for each queue, rounds, measured cells, exit
     `geometry_bad`, `size_bad`, `geometry_debt`, `size_debt`. The exit rows depend on
     the clock; they are a report.

5. N10 (B1), new text of item 1 of I part B1. `geometric_change` is not called when all
   of these are true: the `Adapt` call has a surface target; the trial is an edge split;
   it is not a coarsening and not a repair; and
   - the edge comes from the size queue, or
   - the edge comes from the geometry queue, the estimate of A1 is supported for this
     edge, and `EstimateSaysNeed` is true for it.
   In each other case the guidance check runs as today. Items 2 to 5 of I part B1 stay.
   Counters: `wall_splits_without_guidance` for each of the two queues, and
   `wall_splits_with_guidance`.
   Gates of N10: I "Gates of B1 and B2" items 1, 2, 4 and 5 as written; item 3 is:
   - the G3 fixture two times; validity as in rule 1 (a); the same edits up to the
     smaller count;
   - purpose: in BOTH runs the accepted count at the exit is at least 1.10 times the
     larger accepted count of the two N11 runs. If not: keep the numbers, revert B1 in
     one commit (B2 stays), go on;
   - report: the list of item 4, `guidance_seconds`, `upper_seconds`, and for the splits
     without the guidance check: how many of their new wall faces fail the exact
     measure in `Selection::Add`.
   The G3 runs of the last passed item are the G3 reference for the items after it.

6. The gate texts that this section replaces: I part A "Gates of part A" (G3 by rule
   1); I part B0 "Gate" (G3 by rule 1, G2); I "Gates of B1 and B2" item 3 (items 4 and 5
   above); C section 2, row G3 (rule 1). The old purpose rule "`geometry_bad + size_bad`
   below 7,984" is withdrawn: the exit counts depend on the clock, and a split of one
   failing face can give two smaller failing faces before the error is inside the
   tolerance.

7. Not a task now, for the report only: the 3,508 `deferred` choices. The reviewer
   writes a text for them after the N9 reason counts and the N11 numbers.


User order, 2026-11-16 (LATEST): section 6.5 and R1 clarification

### 6.5 Design steps 8.3 and 8.4 (first part), accepted by the user on 2026-11-16

The user accepted design revision 2 with steps 8.3, 8.4 and 8.4a. Copy these two files
with their SHA256 into your evidence folder and read them completely:

- `/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-NativeIntegrated/f3d988e5-f14f-4a96-b6fb-963ec11730e7/scratchpad/IMPLEMENTATION_8_3_8_4.md` (I)
- `/tmp/claude-1000/-media-rausa-4TB-SU2-Versions-SU2-NativeIntegrated/f3d988e5-f14f-4a96-b6fb-963ec11730e7/scratchpad/ADAPTATION_ONE_ENGINE_DESIGN_V1.md` (D, revision 2.1)

This replaces the line of section 3 "NOT in this work order: design steps 8.3 to 8.7"
for the parts that I names. Steps 8.4a, 8.5, 8.6 and 8.7 stay out.

| Item | Content | Results | Text |
|---|---|---|---|
| N8 | The wall geometry law as production code with its census; the requirements record as a view; the table of the switch places; measurement M6b | identical | I part A |
| N9 | Counters of the wall stage (refusal classes, guidance causes, call sites) and one run of the G3 fixture | identical | I part B0 |
| N10 | No progress veto for a needed wall split | BL wall stage changes; no-BL identical | I part B1 |
| N11 | Inside a failing face the law chooses the edge | BL wall stage changes; no-BL identical | I part B2 |

Order with the cut (6.3):
- These items do not need the cut. They name symbols, not line numbers.
- If the cut has not made its first production edit when you read this: do N9, N10, N11
  and N8 first (in this order: N9 and N10 are the ones that help the BL goal most), then
  the cut. Record the cut references (G1, G2, G3, GQ) after them.
- If the cut is in work: finish the phase that is in work (A, B, C or D) with its
  gates, then do N9, N10, N11, N8, then record G1, G2, G3 and GQ again and continue the
  cut.
- A failed item is reverted by R5 and does not stop the items after it, except: N11
  needs N8 part A1 (the law); if N8 A1 is not in, do A1 before N11. N10 does not need N8.


R1 clarification from the user: A heavy process is a solver rank, a build job, a test binary, or a script of ours that uses more than half of a core for more than 60 seconds. Do not start more than 6 of them in total. A browser or another program of the user is not a heavy process of the work. It does not fail a run and is not a reason to repeat a run. Record it in the receipt as an observation only.

Work order: finish 6.1 and 6.2, then N9, N10, N11, N8 (with N8 A1 before N11 if absent), then 6.3, N6, N7. Do not stop or restart a running job.

# Implementation and validation cadence

User order, 2026-11-16 (LATEST): NIGHT_WORK_ORDER.md section6, including 07:50 B7 decision.

## 6. Addition of 2026-11-16 (user decisions; this section wins over the text above)

State when this was written (07:40): N1 FAILED and reverted (`becb4927c9`), N2 NOT STARTED,
N3 PASSED, N4 FAILED at A1 and reverted (`8039fc1b59`), N5 in its one corrected full
repeat, N6 and N7 NOT STARTED. Do not stop or restart the running job. Finish N5 first.

New order of work: finish N5 -> 6.1 (N1 again) -> 6.2 (N2) -> 6.3 (N4 again) -> N6 -> N7.
All rules of section 2 stay. The rule of 6 heavy processes counts your helper scripts too.

### 6.1 N1 again: the user accepts P7 / P8 with a new reference for event 2

Cause of the first failure (your diagnosis, confirmed by the reviewer): the partition
weights of the next event read `prepared_pieces` of the geometry cache
(`CNativeAffineReference.cpp` `StructuralEstimate`, `CNativeImport3D.cpp` 1762-1806,
MAX over the ranks). P7 changes which facets stay in the cache. So event 2 starts with
other weights and gives another valid mesh. This is a property of the proposal, not an
error of your code. The reviewer's statement "identical results" was wrong for event 2.

User decision: P7 / P8 are accepted when event 1 is identical to P6c and event 2 passes
all standing gates and is repeatable. No physical gate, budget or limit changes.

The "P7 reference" for event 2 is the three event-2 files of your first N1 run
(`R/p7p8_r3_20261115/first_result_failure.json`):

- `adapted_mesh_adap_00002.su2`            63690652de1af70f890994fb5054523403ba1dc27200123a660e573827aebc36
- `adapted_mesh_adap_00002.su2.native_ref` 36c45abe03d1dc38e523583cc3103cfb9c83b17983b3679df0c424fab8ff088a
- `restart_flow_adap_00002.dat`            44766f7f95f14d60011fb27ef1bb864c44e9649f782f2b9a0139c6766c93bc94

Do this:

1. Base: the newest accepted commit after N5. Put back the 16 code / config / test paths
   that `becb4927c9` reverted, with their content of before that commit. Keep the N5
   observation code and the N5 fixtures. A conflict in a test file is your own test code
   (R3). A conflict in a production file: both texts stay, P7 / P8 as written in H and
   the N5 observer as it is; write each such place in the report. No other production
   change.
2. Build. Run the unit cases of the item, then the ordered checks (a) and (b) of H with
   the safeguards (exact first collapse, C492, 481, current target, 2D at 1 / 2 / 4
   ranks). They passed before on this code; they run again because the binary is new.
3. Run ONE full two-event workflow. Gates:
   - all standing gates of both events: 5400 s, 19073 MiB (P8: reported job peak equal to
     the sum of the process peaks, lower bounds not above it), physical validator,
     transfer, continued CFD, isolated restart where H asks it, 95 % timer coverage, join
     agreement, P7 check c1 (owned proof faces SUM equal to the surface size);
   - event 1: three files byte-identical to P6c;
   - event 2: three files byte-identical to the P7 reference above. Equal hashes prove
     that two runs give the same files.
4. If all gates pass but the event-2 files differ from the P7 reference: do not revert
   yet. Run the full workflow ONE more time with the same binary and inputs. If the two
   new runs have the same three event-2 files: N1 is PASSED, these files are the P7
   reference from now, and the report has all three hash sets with the note "differs
   from the first N1 run; the build differs by the N5 observation code". If the two new
   runs differ from each other: the result is not repeatable; revert the item, FAILED.
5. If event 1 differs from P6c, or another standing gate fails: R5 as written.
6. After N1 is PASSED: where H, C or this work order says "six files byte-identical to
   P6c", read "event 1 files identical to P6c, event 2 files identical to the P7
   reference". This holds for gate G5 of C and for N5 / N6.
7. The M18, M20 and M21 rows of the accepted run are qualified. Put them in the N5 table
   in place of the unqualified rows. Report the event times and the timer-tree rows
   beside F3b (2683.4 s and 1977.2 s) and beside the first N1 run (2220.3 s, 1451.0 s).

### 6.2 N2 (B7): the user accepts B7 with new references, by the same rule as 6.1

(Changed 2026-11-16, 07:50, second user decision. It replaces the "limited gate" text.)

1. Compute the condition (item 0 of H 2.0c) with the M18 rows of the accepted N1 run.
   Condition false: write the rank values and the total, N2 is "condition false", go on.
2. Condition true: implement B7 as written, build, run the checks of H 2.0c, then one
   full two-event run (run 1).
3. All standing gates of both events must pass (the list of 6.1 item 3). If one fails: R5.
4. Compare the six files of run 1 with the references of 6.1.
   - All six identical: N2 is PASSED. The references do not change.
   - One or more differ: this is expected, no diagnosis pass. B7 changes what stays in
     the cache, and `CNativeEngine3D.cpp` 2957 reads `prepared_pieces` inside event 1
     too, so event 1 can change as well. Run the full workflow ONE more time with the
     same binary and inputs (run 2). All standing gates must pass in run 2.
     - Run 2 has the same six files as run 1: N2 is PASSED. These six files are the
       "B7 reference" from now. It replaces the references of 6.1 in G5 of C and in N6.
     - Run 2 differs from run 1: the result is not repeatable. Revert B7 in one commit,
       FAILED, and go on with 6.3 on the N1 build.
5. Report in each case: the six hashes of each run, the cell and point counts of both
   events, the event times, the RSS sums, the timer-tree rows and the M18 rows, beside
   the accepted N1 run and beside F3b.
6. The memory rule stays as written in H 2.0c: the job peak of each run must be at most
   19073 MiB. A run above it fails a standing gate (R5; the cause is then in the
   proposal, so B7 is reverted).

### 6.3 N4 again, with the corrected gate G1

C has a new section 2.1 (G1 is an identity gate with its own reference). Read C again
completely and copy it with its SHA256 into your evidence folder.

1. Reference build = the build of the newest accepted commit (after 6.1 / 6.2).
2. Record with it, before the first edit: the G1 reference (C 2.1), and G2, G3 and GQ
   again (the production code changed since `4798ea3936`).
3. Then C 2.1 item 5 decides about A1, and the cut goes on as C says. G5 uses the
   newest references: those of 6.1, or the B7 reference when 6.2 made one.
4. The list "unit tests that fail on the accepted base" goes into the morning report.

### 6.4 Morning report (N7)

Keep the file `Handbook/ADAPTATION_NIGHT_REPORT_20261116.md` and update it in place. Add
one row for each item of this section. The first N1 run and the first A1 attempt stay in
the report as history with their numbers.


User order, 2026-11-15 (LATEST): unattended work, section 2 of NIGHT_WORK_ORDER.md

These rules are the latest user order. Where an older text (AGENTS.md, H Part 0) says
"stop and wait for the reviewer", read it as "stop THAT ITEM, record it, and go to the
next item of section 3 that does not depend on it". Add this order to AGENTS.md.

R1. No gate, budget, tolerance or limit changes. 5400 s for each event, 19073 MiB for the
    job, geometry 1e-4, q / J / size / height gates, requested heights and progression.
    One owned heavy job, nice 19, at most 6 heavy processes on the machine. Check the
    host load and the free memory before each long run.
R2. No redesign and no own fix of production behaviour. You implement what H, D and C
    say. A production change that is not written there is not allowed.
R3. Your own test code is yours to make work (H Part 0). Fixtures, scripts and report
    helpers that you wrote: find the cause, correct, run again, until they test what the
    item asks. Limit: 90 minutes for one fixture. Never change production code or a gate
    to make a test pass.
R4. One source check for a whole item, before its first production edit. Write all
    contradictions in one list in the report.
    - A contradiction on a print-only counter or a report row: write it, continue.
    - A contradiction on a result where the written text has one clear reading that the
      source supports: use that reading, write it in the list as "reading used".
    - A contradiction on a result with no clear reading: do not implement that part.
      If the rest of the item is independent of it, do the rest. If not, skip the item.
R5. A failed result gate (outputs not identical where they must be, accept / reject,
    validity, geometry, quality, memory, time):
    - Preserve the evidence. Write the first number that differs.
    - Use ONE diagnosis pass of at most 60 minutes, read-only plus the existing counters,
      to name the cause. If the cause is a transcription or wiring error against the
      written proposal (your code does not do what the text says), correct it one time
      and repeat the failed check one time.
    - If it fails again, or the cause is in the proposal itself: revert the item in one
      commit (keep the evidence and the reverted commit hash in the report), mark it
      FAILED, and go to the next item that does not depend on it.
R6. Time limits, so that one item cannot take the whole night: 90 minutes for one
    fixture (R3); 60 minutes for one diagnosis (R5); 3 hours for item N3. At a limit:
    write the state, go on.
R7. Keep a progress file `R/night_20261115/progress.md` (R as in H). Update it after each
    sub-step: item, sub-step, commit, gate numbers, next action. After each compaction of
    your context, read this work order and the progress file again before you continue.
R8. Commit after each passed sub-step. Push the checkpoints. Never commit a state that
    fails its gates as "accepted"; a preserved failure is labelled as a failure.
R9. Readable code (D 7.1a) for all new code. Written exceptions are listed in C.

User order, 2026-11-15 (LATEST; replaces the "one correction" rule for fixtures):
Your own test code is yours to make work. A test fixture, script or report helper that
you wrote for an item is not production code and not a result gate. When it fails
because of its own setup: find the cause, correct it, run it again, and repeat until it
tests what the item asks. No limit on the count; limit of 90 minutes for one fixture.
Never change production code, a gate or a tolerance to make a test pass. Write each
cause and correction in the report afterwards. Do not stop and do not wait for the
reviewer. Stop only when: (1) a result gate fails in a production run; (2) the source
contradicts a proposal on a result; (3) the corrected fixture shows that the production
code does not do what the item asks; (4) the 90 minutes end.

User order, 2026-11-15 (LATEST; refines the "stop at the first contradiction"
rule below): before any production edit, source-check the WHOLE item (all its
steps, checks and counters, including connected items built together) and report
ALL contradictions in ONE list with source lines/numbers; the reviewer answers
them in one correction. A mismatch in a print-only counter or report row does not
stop the item: report it with call-site counts and continue. Gates on results
(mesh/output identity, accept/reject decisions, validity, geometry, quality,
memory, time) stay strict: stop on their first failure. Still no attempt loops
and no independent redesign.

Reviewer correction supplied 2026-11-15: M6_FIX_HANDOFF.md section1.7 (P6c)
supersedes waiting after P6. Same-fixture baseline scan first; implement the
specified deferred-bound completion/counter/guard records and repeat the ordered
checks once, stopping on first failure. No independent replacement fix.

User order, 2026-11-15 (updated M6_FIX_HANDOFF.md): the reviewer supplies ideas
and design; implement, build, run and report, without independent redesign.
Check proposals against source first. On a contradiction or first failed check,
stop that item, report source lines/numbers and available purpose/call-site counters,
and wait for a corrected proposal. No attempt loops or self-invented follow-up fixes.
P6 is isolated before the cheap group: set aside its open edits, implement section
1.5, then run section 1.6 checks once in order, stopping on the first failure.
No full M6 run until the preceding P6 gates pass. Do not change pinned fixture
values; report changed counts/ranges. Keep F1's build494 count reference until
P6's full two-adaptation milestone passes. Reviewer-owned one-engine design is
read-only; after cheap-group profiling and F3, source-check it and measure M1–M8.
No implementation of its steps8.2–8.7 before user acceptance.

User order, 2026-11-14 (M6 fix handoff): protect shared-engine changes with the
executed gates in `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57/shared_engine_regression_gates.json`.
For each connected shared-code change, run the affected no-BL first-collapse
work/count gate, exact saved-state BL repeat and native 2D BL/no-BL checks at
1/2/4 ranks. Run the full two-adaptation no-BL workflow at integration milestones.
Keep the stated time/work/resource and physical gates unchanged, and report failed
or unevaluated gates explicitly. Artifact hashes alone prove files unchanged;
they do not establish execution or performance preservation.

User clarification, 2026-11-11 (LATEST CADENCE; supersedes earlier immediate-full-run retries):
Use incremental, stage-based integration. Establish the execution sequence/timing
from source and existing evidence. Work in order: saved CFD/geometry; composed
metric/normalization (finish tangential-profile investigation); coupled surface
preparation; constrained directions/first row; successive growth; outer transition
and strict closure; transfer/continued CFD/second adaptation/isolated restart.
Define each stage's input/output/acceptance before changes. Save credible reusable
stage outputs with complete provenance and enough state to reconstruct the SAME
composed target and geometry; sensor tensors alone are insufficient. Preserve
validity-first provisional outputs with diagnosed quality debt; final gates still
apply before mesh acceptance. Use focused checks and meaningful MPI integration
milestones. Reserve full M6 retries for integration milestones or failures that
cannot be reproduced at a stage boundary. Keep one cycle, one owned heavy job,
budgets, source freezing and the complete final objective unchanged.
Maintain Handbook/ADAPTATION_TIMINGS.md with current per-stage timings/work and
evidence-backed major computational efficiency improvements; keep nested timers,
contention and attribution gaps explicit.

User clarification, 2026-11-11: RANS500k is a NOMINAL metric-complexity target,
not an acceptance gate or exact node count. Explain overshoot and uncertainty;
preserve requested normal heights/progression and all strict acceptance/resource gates.

User clarification, 2026-10-30: the 500k adaptation target was intended for RANS,
not Euler. Use a much smaller Euler target; the current functional-loop attempt
uses 50k (an implementation choice), with 500k reserved for RANS. Keep all geometry,
quality, transfer and requested-height gates strict. Report actual mesh point/cell
counts separately from the metric-complexity target.

User instruction, 2026-10-29: let the complete M6 attempt drive development.
Freeze a coherent implementation, allow generous but bounded certification work,
and attempt the requested CFD → mesh → CFD → mesh workflow. Address construction,
certification or transfer/CFD according to that run's failure; focused checker
changes and tests are diagnostics, not the immediate capability objective.

Permanent user instruction, 2026-10-29 (overnight continuation): if the M6 run
reveals a clear computational bottleneck whose fix will improve efficiency, or a
construction/certification/transfer/CFD error, preserve its evidence, stop only the
verified owned run, implement a connected fix, validate the affected behavior and
rerun ONERA M6. Continue autonomously until a valid accepted Euler
CFD → mesh → CFD → mesh loop succeeds; passing diagnostics are not completion.
Then verify that BL generation still works before proceeding with the RANS grid
and the same target-driven workflow. Keep strict acceptance gates, one owned heavy
workload, resumable evidence and regularly pushed reviewable checkpoints. The user
is unavailable overnight; resolve routine choices within the authorized scope
without waiting for answers. This instruction persists across sessions/compaction.

User instruction, 2026-10-12: prioritize substantial, connected implementation
steps over repeated small checkpoints.

- Group changes that solve the same end-to-end capability before running a broad
  regression. Do not require the full suite after each small edit or checkpoint.
- During implementation, run focused tests when they resolve a concrete failure,
  exercise new risky behavior, or validate a changed interface. Documentation and
  routine low-impact edits do not need a new test campaign.
- Run appropriate broader regression at meaningful integration milestones and
  before declaring the integrated result validated. Never weaken acceptance gates
  or omit necessary correctness checks to satisfy this cadence.
- Reserve MPI campaigns for substantial connected implementation milestones; do
  not repeat them for each small local edit. Use focused serial checks during
  development where they resolve a concrete risk.
- Push reviewable checkpoints regularly. Clearly label incomplete or unvalidated
  work; a checkpoint does not itself require repeating a previously passing suite.
- Updates should state progress toward the actual end-to-end goal and its remaining
  blockers, rather than treating each passing intermediate test as a milestone.

## Capability-first implementation

Permanent user instruction, 2026-10-14; applies across context compaction and
future sessions.

- Organize work around a complete end-to-end capability and its known difficult
  failure, rather than repeatedly extending isolated operators until toy cases pass.
- Before a substantial implementation, state the missing capability, the evidence
  for its underlying limitation, and the acceptance conditions. Choose a connected
  design that addresses that limitation across the affected callers and constraints.
- Use the known difficult target and a small set of meaningfully different fields
  or geometries to assess the complete change. Do not defer the difficult case
  until a succession of easier checkpoints has passed.
- Simple tests establish primitive correctness or resolve specific risks; they do
  not establish robust mesh generation and must not be reported as capability
  milestones. Keep private intermediate progress distinct from an admissible final
  mesh and a successful adaptation/solution-transfer workflow.
- The next meaningful curved-construction milestone is a complete accepted mesh
  under unchanged constraints. If the current recovery cannot deliver it, reconsider
  the construction strategy before adding another local fallback.
- When a strategy stalls on the difficult target, preserve the diagnosis and
  reconsider the construction or coordination it lacks. Do not keep adding local
  variants or raising limits without evidence that they address the blockage.
- Deliver substantial connected improvements, then validate at the cadence above.
  Preserve the full agreed goal and all acceptance gates; neither a prototype nor
  a passing fixture justifies narrowing the goal or declaring it complete.

## Reuse and resumable validation

Permanent user instruction, 2026-10-13; applies across context compaction and
future sessions.

- Reuse existing implementation and validation facilities before adding new ones.
  Keep implementation and checks tied to the full agreed goal and its exclusions.
- Reuse passing evidence when its scope covers the requirement and the relevant
  source, build/binary, configuration, inputs and validation helpers remain valid.
  Verify the needed provenance; source hashes alone do not prove unchanged runtime
  behavior. Rerun checks affected by changes, failures or unresolved concerns.
- Make long validation campaigns resumable: save complete independently validated
  cases or stages with their provenance as they finish. A timeout must not discard
  completed evidence or force successful work to repeat. Partial results cannot
  establish whole-campaign success.
- Resume a saved result only after verifying its relevant hashes and acceptance
  conditions. Preserve failures and diagnostics, keep resource/work limits explicit,
  and require every planned case or stage before declaring completion.
- Before restarting a job, verify its existing process/session/job handle. Elapsed
  time, an observation timeout or stale logs alone do not establish termination.
- Keep compiled/executing/pinned C++ sources unchanged throughout a build or validation
  campaign. Verify the owned handle is terminal before editing those sources. Git
  snapshots and documentation updates are allowed without changing the pinned code.

## Adaptation architecture

User instruction, 2026-10-14: Euler-to-BL and BL-to-Euler describe validation
scenarios, never production transition modes. Mesh construction and removal must
follow the target metric, geometry authority and currently requested wall-height
constraints through general remeshing operators. Do not dispatch reconstruction
from Euler/RANS labels, an inferred input-mesh class, case names, saved coordinates
or fixture identities. Solver compatibility and solution-transfer checks remain
separate from mesh reconstruction. Wall-aware operators are allowed to enforce
actual geometric constraints; limited fixture success does not establish general
robustness. Qualify wider fields/geometries rather than adding scenario switches.

## Handbook maintenance

User instruction, 2026-10-14: keep HANDOFF_Codex.md current and concise. Put detailed
current topics and accumulated history into smaller linked handbooks when the entry
point grows. Preserve findings, failures and evidence; historical job/status notes
must not be mistaken for current state. Update the current state in place rather
than prepending repeated copies of prior updates.

## Resource discipline

User instruction, 2026-10-17: account for machine contention and RAM usage.

- Check actual host load and available RAM before sustained computation. Run one
  owned heavy job at a time; use low priority,2–4 build workers when resources permit
  (user update2026-10-29) and single-threaded
  libraries. Keep local MPI correctness checks bounded to available ranks/resources.
- Check live job handles before starting another job. Observe process memory during
  longer runs, preserve resource receipts, and leave unrelated processes untouched.
- Use the repository build's compile-tmp on the4TB disk while the root/tmp filesystem
  is nearly full. Do not infer whole-job peak memory from one process's RSS.

## Optional local second opinion

User authorization, 2026-10-18: consult the local Claude Code installation when
useful, using Opus 5.5 at medium or high effort. Keep source-checking and validation
with the implementation; a model review does not establish correctness.

## Staged adaptation and boundary mobility

User reaffirmation,2026-10-26: implement the already agreed validity-first construction
followed by metric/BL-aware quality improvement. q/J/size and requested first-height gates apply to the
final accepted mesh; intermediate meshes must remain valid. Fixed physical boundary
node coordinates are not required. Allow redistribution on the prescribed geometry
while retaining genuine features, body separation and requested BL constraints.
Complete the connected distributed workflow rather than accumulating strict local
construction fallbacks; preserve all final acceptance/transfer/restart obligations.


User clarification,2026-10-26: construction must omit q/J measurements, shape
ranking and quality-progress vetoes that are unnecessary for valid elements.
Exact positive, nondegenerate cells, topology, geometry and body separation remain
mandatory; J>=.001 is a quality threshold, not geometric validity. Measure and
improve shape in the subsequent stage and enforce all final acceptance gates.
Delegate bounded routine tasks to more token-efficient models; retain integrated
design, source review and acceptance assessment with the primary agent.
