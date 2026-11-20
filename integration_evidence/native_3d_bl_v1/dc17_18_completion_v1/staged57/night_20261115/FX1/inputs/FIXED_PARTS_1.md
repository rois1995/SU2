# Fixed parts 1 (item FX1), version 3: no second build of the mobility policy, exact skip in the metric gradation

Author: reviewer, 2026-11-19; version 3 of 2026-11-20, 22:15. For the main agent. The user sends it; it is a user order.
Start: this order came while the second ON run of `FP1_RESUME_6.md` was running. Do NOT stop that job (R12). First
finish and close resume 6 as its text says. Then start this item, without waiting for the user:
- if resume 6 closed with "ON accepted": the whole item as written;
- if resume 6 closed with "FP1 ON: FAILED": parts P and M with the gates of 5.1 and 5.2 only (the option OFF). Skip
  section 5.2b and the start rule of section 1 item 3 (with the option OFF the coverage has room; do part P). Where
  5.3 names the ON run, read it without that run. Write "G5 ON pending" in the report.
Base: the commit that closes resume 6. FP1 does not touch the lines of parts P and M; with its option OFF the results
are those of the reference in force.
Version 3: the timer scopes of version 1 (part N) and three more (part S) are DONE in the item FP1: N1 to N4 in
`d76fa0d5bc`, S1 to S3 in the commit "FP1 S" of resume 6. This item has the two parts that are left: P and M.
All rules of section 1 of `AGENTS.md` apply (R1 to R13). R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
Evidence folder: `R/night_20261115/FX1/`. NRR = `Common/src/adaptation/native_engine3d/NativeRemeshRun.inc`.
Line numbers are those of commit `a59380df18`. The files of parts P and M (`CNativeBoundaryParameters3D.hpp` / `.cpp`,
`CNativeReferenceIO3D.cpp`, `CNativeAffineReference.cpp`, `CSolver.cpp`) did not change from that commit to
`23ef6e9d3f` ("FP1 S"). If a line differs: find the place by the statement that is named.

This is an item with IDENTICAL RESULTS: the six G5 files, all decisions and all integer rows stay equal. Only seconds
change, and the timer rows that section 5 lists. Two parts, in the order P, M. They are independent of each other.
Part P removes named time; the names that section 2a of `AGENTS.md` asks for first are in the code (parts N and S of FP1).

Numbers below are from the reference run of N22 part C,
`R/night_20261115/N4/section6_3_after_N11/round_8_5u_C/G5/full591_run1` (6 ranks; events 1 / 2).

## 1. Why

| Fixed part | Seconds, event 1 / 2 | Fact |
|---|---|---|
| Return of the mesh: second build of the mobility policy | 16.95 / 17.86, all ranks | row `transfer / mobility_policy` |
| Reference file output: third build, on rank 0; ranks 1 to 5 wait | 18.01 / 18.62 | row `output / mobility_policy`, root `mesh_output` |
| Metric: gradation sweeps | 39.2 / 161.2, and 128.8 for the metric after event 2 | 99 % of `ComputeMetric`; the wall time is the compute of the busiest rank, the other ranks wait for it at each sweep |
| Timer coverage (gate 95 %) | in that run: smallest 95.29 % / 95.23 %. Build615 (scopes N1 to N4): option OFF 97.467 % / 98.019 %; option ON 96.952 % / 94.84 % (event 2 failed). Build616 (with S1 to S3), option ON, first run: 99.041 % / 98.658 % | there is room for part P now, also with the option ON |

1. `BuildMobilityPolicy` (`CNativeAffineReference.cpp:1462-1527`) is a pure function of the reference geometry object,
   the angle and the marker roles. It does not read the surface mesh, the boundary parameters or a tolerance. Each rank
   holds one geometry object for the whole run. The state already holds the result (`BoundaryParameterState::mobility`,
   written only at `CNativeBoundaryParameters3D.cpp:246` and `:626`). The later builds compute the same value again and
   compare it with itself.
2. The gradation (`SU2_CFD/src/solvers/CSolver.cpp:3151-3244`) visits a point again when a neighbour changed, and then
   tests ALL its neighbour pairs again. For a pair whose two tensors did not change, the test has the same inputs as
   the last time and gives the same answer ("satisfied").
3. The coverage gate reads root `remesh`, boundary `final`. Part P removes 17 to 18 s of named time there. Rule for
   the start of part P (section 2a of `AGENTS.md`): in the two ON runs of resume 6, files
   `R/night_20261115/FP1/resume6/G5/ON1/full616_run1/f3b_event2_timer_gate.json` and `.../ON2/...`, for each rank take
   `root_seconds` and `unattributed` of the ledger summary and compute
   (`root_seconds` - 18 - `unattributed`) / (`root_seconds` - 18). If the smallest of the 12 values is below 0.955: do
   not implement part P; do part M alone and report (the reviewer then gives more names first). In that case the
   checks "the row is absent" of 5.2 and 5.2b do not apply: the rows `transfer / mobility_policy` and
   `output / mobility_policy` then stay as in the reference. Expected: the smallest value is about 0.986, so part P
   is done.

## 2. Timer scopes: done in FP1

The scopes N1 to N4 (`d76fa0d5bc`) and S1 to S3 (resume 6) are in the code. Do not change them in this item. The
section numbers below are those of version 1.

## 3. Part P: the mobility policy is built one time for each rank

### 3.1 P1: the return path (`Rebind`)
`BoundaryParameterState::Rebind` (`CNativeBoundaryParameters3D.cpp:719-736`) makes a new state from `*this` with the
SAME `geometry` pointer and the SAME `mobility`. The constructor (`:224-301`) then builds the policy again and compares
(`:250-253`).

Change: add ONE trailing parameter to the public constructor, in the declaration
(`Common/include/adaptation/CNativeBoundaryParameters3D.hpp:88-91`) and in the definition (`.cpp:224-226`):
`const SU2NativeGeometry::MobilityPolicy* proven_policy = nullptr`.
Meaning (write it as the comment): a policy that is equal to `BuildMobilityPolicy` of a geometry object whose inputs of
`BuildMobilityPolicy` are bit-equal to those of `geometry`.
- In the constructor, in the place of lines 251-253: if `proven_policy` is given, throw the same `invalid_argument` as
  today when `EncodeBoundaryAtlas({}, *proven_policy) != EncodeBoundaryAtlas({}, mobility)`, and do NOT call
  `BuildMobilityPolicy`. If it is not given: lines 251-253 as today.
- P1: `Rebind` (`:735`) passes `&mobility`. Its `geometry` argument is its own member, so it is the same object.
- No other caller passes the parameter, except the one of 3.2. `FromPayload` (`:716`), `LoadReference` and each other
  route build and compare as today. The corner check of `:247-248` and each other check of the constructor stay.
- No new member in `BoundaryParameterState`, `MappedFacetReference` or `GeometryStatsState` (their sizes are in the
  modeled bytes). A defaulted parameter adds none.
- Checked by the reviewer: the budget of `Rebind` (`:729`) is local and discarded, and the skipped build has no other
  effect. If your source check finds a value of the skipped build that is read later (a printed row, an admission):
  write it in the list and do not implement P1 (R4).

### 3.2 P2: the check of the written reference file
`EncodeReference` decodes its own text again as a check (`CNativeReferenceIO3D.cpp:186`, `DecodeReferenceImpl`). The
decoder makes a new geometry object from the text (`:286`) and the constructor builds the policy a third time (`:291`).

Change, for the check call inside `EncodeReference` only (not for `LoadReference` or any other caller of
`DecodeReferenceImpl`). `EncodeReference` has three callers (the reference file output and the two target checkpoints
`CNativeTargetCheckpoint3D.hpp:123`, `:214`); the change holds for all three, it is exact for each.
1. `DecodeReferenceImpl` (a static function of this file) gets one more argument, default "none": the in-memory state
   `proven` that `EncodeReference` just encoded.
2. After the decoder made its geometry object (`mapped`, line 286): only when `header.version >= 8`, `proven` is given,
   `proven->original->Parameters()` exists and its `Mobility().identity != 0`: let `g` =
   `proven->original->Parameters()->Geometry()` and `d` = `*mapped`. Compare `d` and `g` bit for bit (the bits of each
   double; a signed zero is a difference; field by field, never `memcmp` of a struct, structs have padding):
   - `Reference().Facets()`: the same count; in the order of the ids: id, marker, the three `v[k].id`, the nine coordinates;
   - `Curves()`: the same count; in the order of the ids: id and its 10 control points;
   - `Reference().Affine()`: the same count; in the order of the ids: id, identity, version, dimension, origin x / y / z,
     normal x / y / z, maximum_error;
   - `Version()`.
   This is each input that `BuildMobilityPolicy` reads (`CNativeAffineReference.cpp:1462-1530`); its other members
   (`edges`, `edge_facets`, `nodes`, `body_roots`) are made from the facets only (`:1285-1299`). Public accessors are
   enough (`Geometry()`, `Mobility()`, `Reference()`, `Curves()`, `Version()`, `Facets()`, `Affine()`); add no member.
   Confirm the list in your source check and write it in the report.
3. If all are equal: the decoder passes `&proven->original->Parameters()->Mobility()` as `proven_policy` to the
   constructor (line 291). The constructor then compares the decoded policy with it (3.1) and does not build. The
   decoded angle and roles are checked by this encoded comparison.
4. If one differs, or the conditions of item 2 do not hold: the decoder passes nothing; build and compare as today.
Reason (comment): the in-memory policy was proven against a geometry object with these exact inputs, and the build is a
pure function of them. The check of the file stays as strong as today.
- Checked by the reviewer: `decoded_peak` (`:186`) and the peak of the decoder's reservation (`:294`, `:299`) reach only
  a local that is discarded. If your source check finds a number of this decode that is NOT discarded: write it in the
  list and do not implement P2 (R4).

### 3.3 What must not change
The event-1 build in the preparation (it is the first and necessary one; the row `preparation / mobility_policy` keeps
the calls that it has today in each snapshot: 3 in event 1 and in the G2 / GQ prefixes, 2 in event 2 of G5). `PrepareMobility`. The content of both output files. Each validation that is not the repeat of the build.

## 4. Part M: exact skip of unchanged neighbour pairs in the gradation (3D only)

Place: the lambda `gradeNativeMetric`, `SU2_CFD/src/solvers/CSolver.cpp:3151-3244`.

Today, for each active point: line 3186 computes `inverse[point]` from `previous[point]`; the loop of line 3193 tests
each neighbour.

Change (only when `nDim == 3`; with `nDim == 2` the code runs exactly as today, because the 2D test reads `current`):
1. Before line 3182 resets it, keep `const bool own = changed[point] || sourceChanged[point];`.
2. If `own` is false: skip the eigen decomposition of `previous[point]` and the loop that fills `inverse[point]`
   (lines 3186-3191). Keep the declaration of line 3185 (`vec`, `val`, `work` are used again at line 3230). The stored
   `inverse[point]` is from the last visit of this point, and `previous[point]` did not change since then.
3. If `own` is false: in the neighbour loop, `continue` at once for a neighbour with `!sourceChanged[neighbor]`.
4. Nothing else changes: the counters `totalGradedPoints` and `totalTensorUpdates`, the order of the neighbours, the
   arithmetic of each pair that is tested, the lines after the loop.

Proof (write it as the comment): a point with `own == false` is active only because a neighbour changed. Take a pair
(point, n) with `sourceChanged[n] == false`. The 3D test reads `inverse[point]`, `previous[n]`, the coordinates and
`growth`. None compares unequal (`!=`, the test that makes points active today) since the last evaluation of this pair:
a change of `n` makes `sourceChanged[n]` true in the next sweep, so the pair is evaluated then; a change of the point
makes `own` true; sweep 0 of each trial sets `sourceChanged` for all points. That last evaluation said "satisfied"; else
`changed[point]` would be set and `own` would be true now. A satisfied pair does nothing (`continue`, line 3218). So the
skip does the same. (The only change that `!=` does not see is the sign of a zero.)

`CSolver.cpp` is built with `-ffast-math`: a changed loop can change bits even when the source order is equal. So the
proof of this part is the gate. The metric fields (`Metric_XX .. Metric_ZZ`) are in three files:
`restart_flow_adap_00000.dat` (the first metric; it is NOT one of the six G5 files), `restart_flow_adap_00001.dat` and
`restart_flow_adap_00002.dat`. All three must be identical: the first has SHA256
`61ebd3b0cd3a00889784772378ce7897ec6245c517f2ee382492288c43dbafd4` (the same in the runs of N21 W, N22 M, N22 C and in
the G2 / GQ cases), the other two are G5 files. And the printed counts of the gradation must be equal (5.2 item 4).
If not: R5, revert part M only. No new print in this part.
Expected (a report): the saving is a part of the compute of the busiest rank; it is not known before the run.

## 5. Gates

"Reference" = the references in force of sections 2 and 2a of `AGENTS.md`.

### 5.1 After each part (P, then M): the short gates
1. Build without a new warning. `git diff` of the part: only the places that its section names (and test code). For
   part P these are: the constructor declaration in `CNativeBoundaryParameters3D.hpp` and its definition in
   `CNativeBoundaryParameters3D.cpp` (one trailing defaulted parameter, the lines 250-253), `Rebind` (`:735`); in
   `CNativeReferenceIO3D.cpp` the forward declaration of `DecodeReferenceImpl` (`:97`, it holds the default argument),
   its definition, the check call in `EncodeReference` (`:186`), the constructor call (`:291`) and one new static
   helper function in this file for the bit comparison of 3.2 item 2. For part M: the lambda `gradeNativeMetric`.
2. G1: tables identical. G3: the rule for an item with identical results.
3. G2 and GQ with the comparator of N22 part C (`R/night_20261115/N22/compare_prefix.py`, `part == 'C'`): all integer
   rows, all modeled bytes and all timer rows that this comparator checks exactly are equal to the reference.
   `communication` rows and the collective, election and tracked-call counters are a report in G2 and in GQ, as in
   N22 part C (this is also the rule of correction 1 of work order 3).
   The calls of the row `construction / scheduling`: your comparator copy of resume 6 takes this row out of the strict
   list. Add one check (test code, a copy in `R/night_20261115/FX1/`): for each of the 18 records of
   `R/night_20261115/FP1/resume6/scheduling_calls_reference.json` (case, snapshot, rank), the calls of this build are
   equal to the field `calls` of the record (8,137 at `length_collapse_exit` in G2 and GQ; 115,680 at the first
   `construction_entry` in GQ). All other rows as before.
   Expected: no difference from the G2 and GQ comparisons of resume 6. In particular `preparation / mobility_policy`
   keeps `calls=3` and no new row appears.
4. The safeguards of section 2 of `AGENTS.md` for a change of shared code (part P changes reference code; part M
   changes the metric code that the 2D cases use): each check passes with the same table of cases and
   pass / fail as in its last accepted run. G1 holds the `ComputeMetric` tests in 2D and 3D.
5. For the build with part M (not for the build with part P alone): in the G2 and the GQ case, `restart_flow_adap_00000.dat` has the SHA256 of section 4, and the lines
   "Native metric work", "Native gradation tensor updates" and "Native sensor gradation" of `run.log` are equal to those
   of the reference case, except for the seconds. If not: R5 for part M.
Commit and push after each part. A commit of a part says "G5 pending", not "accepted" (R8).

### 5.2 After the two parts: ONE G5 run with the option of FP1 OFF (its default)
1. All standing gates of both events. Runner: a copy in `R/night_20261115/FX1/` of your G5 runner of FP1 (the one of
   resume 5 and 6, which runs OFF first and then binds the ON run to the OFF proof of the same build); NOT the runner
   of N22 part C. Its time comparisons are a report in this item. The copy does not write
   `R/night_20261115/FP1/on_reference.json` (that file exists). Add to the copy the checks of this section that the
   runner does not have: the SHA256 of `restart_flow_adap_00000.dat`, the gradation counts, the absent rows (R3).
   Six files identical to the reference in force, and `restart_flow_adap_00000.dat` with the SHA256 of section 4.
2. Timer rows at root `remesh`, boundary `final`, each rank, both events:
   - coverage at least 95 % (the gate). Report it beside the values of the G5 OFF run of build615;
   - the row `transfer / mobility_policy` is absent (part P1). This is intended; a row with zero calls is not printed;
   - `fixedstorage=4120`.
3. Root `mesh_output`, rank 0: the row `output / mobility_policy` is absent (part P2).
4. `run.log`: the counts of the gradation of the three metric computations are equal to those of the reference run:
   event 1: 7 trials, 800 sweeps, 14153877 visits, 9853577 tensor updates; event 2: 8, 1478, 43004068, 30146553;
   after event 2: 8, 1199, 41122066, 28821833.
5. Report (not a gate): the seconds of `transfer`, of the reference file output, of `target_composition` /
   `nested_gradation` for the three metric computations, the native maximum of each event, beside the reference
   run and beside the G5 OFF run of build615 (native maxima 1,778.627 / 1,029.081 s). A time claim needs two runs one
   after the other (section 2a); one run is enough for this item because each saving is a removed computation.

This run is also the G5 OFF proof of part S of FP1 (resume 6 left it "G5 OFF pending"): write that in the report.

### 5.2b Then: ONE G5 run with the option of FP1 ON (`ADAP_NATIVE_LOCAL_CORE= YES`), the same binary
1. All standing gates of both events. Six files identical to the reference for the option ON,
   `R/night_20261115/FP1/on_reference.json` (parts P and M change no decision, also with the option ON).
2. Coverage at least 95 % on each rank, both events; `fixedstorage=4120`; the rows `transfer / mobility_policy` and
   `output / mobility_policy` are absent.
3. `run.log`: the gradation counts of the three metric computations are equal to those of the ON runs of build615
   and of resume 6 (trials, sweeps, visits, tensor updates): event 1: 7, 800, 14153877, 9853577 (as OFF); event 2:
   8, 1056, 28977415, 21001471; after event 2: 8, 1246, 28674333, 20088842. If the run that
   `FP1/on_reference.json` names as its repeat case has other counts: use the counts of that run and report it.
4. The time rule of an ON run against the OFF run is a report in this item. Report the native maximum of each event
   beside the two ON runs of resume 6.
If this run fails: section 5.3.

### 5.3 If a G5 run fails (R5, with this addition)
One diagnosis pass names the part, in this order:
1. A restart, mesh or reference file differs, or a gradation count differs: part M (only part M can change a number;
   look at `restart_flow_adap_00000.dat` first). Exception: only the `.native_ref` files differ and each restart and
   mesh file is equal: part P.
2. A run error that names the mobility policy or the decode of the reference, or a printed row
   `transfer / mobility_policy` or `output / mobility_policy`: part P.
3. Coverage below 95 %: part P (it removes named time).
4. Another failure of a timer row: part P (part M has no timer row).
The same order holds when only the ON run fails.
If the diagnosis finds a transcription error in that part: correct it one time (R5). Else revert that part only (one
commit; the parts are separate commits on separate files). Both make a new binary. So in both cases, one time: build,
the short gates of 5.1 for the new build, then the G5 OFF run and the G5 ON run again (the ON run needs the OFF proof
of its own build). If a run of this repeat fails: revert the item (both parts), and report; no more runs.

## 6. Report and close

`R/night_20261115/FX1/reports/FX1_<build>.md`: the source check list (R4) with the list of build inputs of 3.2; the
gates of each part; the coverage table of build615 OFF / this run for each rank and event; the seconds of 5.2 item 5; each R3
correction; each question (R13). Update the progress file, `HANDOFF_Codex.md`, `HowToProceed.md`,
`Handbook/ADAPTATION_TIMINGS.md` (the rows of the three fixed parts; the note that the coverage has more room now).
Commit, push, verify that no owned job runs, stop and wait for the user.

## 7. Not in this item

A faster `BuildMobilityPolicy` itself. The validation of the returned state and of the written file (apart from the
repeated policy build). The balance of the gradation over the ranks, fewer complexity trials, another gradation
method (these change results or move data; they need a decision of the user). The metric computation after the last
event (its tensors are in a G5 file). The seed census, the final proof and their face memo. Any gate, tolerance or
default.
