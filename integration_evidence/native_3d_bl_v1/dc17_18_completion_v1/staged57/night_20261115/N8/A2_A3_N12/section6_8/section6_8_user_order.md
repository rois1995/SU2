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
