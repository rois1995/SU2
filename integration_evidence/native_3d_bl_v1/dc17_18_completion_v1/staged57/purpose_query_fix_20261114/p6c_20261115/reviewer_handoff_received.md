# Handoff to the main agent: fix the no-BL regression now, then the proposed fix for each problem found

Repo: `/media/rausa/4TB/SU2_Versions/SU2_NativeIntegrated`, branch `codex/native-3d-bl`.
R = `integration_evidence/native_3d_bl_v1/dc17_18_completion_v1/staged57`.
Eng = `Common/src/adaptation/CNativeEngine3D.cpp`, Bnd = `Common/src/adaptation/CNativeBoundary3D.cpp`,
BP = `Common/src/adaptation/CNativeBoundaryParameters3D.cpp`, AR = `Common/src/adaptation/CNativeAffineReference.cpp`,
Dist = `Common/src/adaptation/CNativeDistributed3D.cpp`, Pipe = `SU2_CFD/include/adaptation/CNativeBLPipeline3D.hpp`,
Tgt = `SU2_CFD/include/adaptation/CNativeBLTarget3D.hpp`.

Annex with all the evidence: `M6_STAGE3_WALL_FIRST_DESIGN_BRIEF.md` (sections 3.1 to 3.9).
Reports: `Handbook/ADAPTATION_M6_MEASUREMENT_V2_20261114.md`,
`Handbook/ADAPTATION_M6_NOBL_CLEAN_CHECK_20261114.md`,
`Handbook/ADAPTATION_M6_NOBL_CAUSE_CONFIRM_20261114.md`.

## 0. How to read this file

- Part 1 is an ORDER of the user (2026-11-14): fix the regression now.
- Part 2 is the reviewer's PROPOSED fix for each problem. These are proposals, not user
  decisions and not tested.
- Part 3 is the order of work.
- Roles (user order, 2026-11-14): the reviewer supplies the ideas and the design. You
  implement, build, run and report numbers. Do not redesign. Check each proposal against the
  source before you implement it. If the source or a measurement shows that a proposal is
  wrong, stop that item, report the line or the number, and wait for the corrected proposal.
- No attempt loops (user order, 2026-11-15: "I don't want the main agent to wander around
  trying fixes one at a time, I want precise and targeted fixes that well blend in the code
  and procedure."). For each item: implement the change as written, run its checks once, and
  report. If a check fails, do NOT add a second change of your own (memo quotas, extra
  reuse, new cache keys, new special cases). Stop the item, report the failed number with
  the counters split by call site, and wait for the corrected proposal. Attempts 3 to 7 of
  the P5 repair are the pattern to avoid: they cost five builds and removed 0.45 % and 0 %
  of the work.
- No new full M6 run before the reviewer hands over proposal P6 (section 1.6, to come).
- All AGENTS.md rules stay in force. No gate, budget or limit changes: 5400 s total, 12 GiB,
  geometry 1e-4, q / J / size / height gates, requested heights and progression, CAD optional.
  One owned heavy job, nice 19, at most 6 heavy processes on the machine. The gathered serial
  MMG path stays.
- Status letters: M = measured, C = the reviewer read the code, I = inferred, U = not checked.
- A fix is "results-identical" when the mesh and the accept / reject decisions do not change.
  Prove it with a hash or with equal counters, not with a statement.

## 1. Do now: the regression of the no-BL path (user order)

### 1.1 Facts (M, C)

- Commit `76f10c8310` made the first collapse pass of the no-BL M6 case 19 times more
  expensive in upper-bound work (54,229,713 -> 1,034,769,513 units; same accepted edits). A test
  build with the old code at one location gave 78,172,576 units. Both adaptations of the case
  now reach the 5400 s limit and are rejected.
- The location: the `tighten` lambda of `BoundaryParameterState::EntityFaceError`
  (BP:368-378). Each face above 1 x tolerance gets a `SurfaceBoundPurpose::ErrorMagnitude`
  query. This query must close the gap between upper and lower bound to one tolerance
  (AR:2341-2350).
- The old code cannot return: the BL surface stage needs the magnitude for accepted edits
  (build 481 correction).
- The callers ask three different questions, but all get the magnitude:
  1. "Is the error <= tolerance?": Eng:1948, Eng:5362, Eng:6001, Eng:6957,
     `Common/src/adaptation/CNativeRegion3D.cpp:243`.
  2. "Is the sum of excess^2 above a limit?": `GeometryGuidanceExcess(..., reject_above, ...)`
     (Bnd:3466-3509) from Eng:4001 (coarsening, limit 0), Eng:4014, Eng:6632, Eng:6967, Eng:7113.
     The limit stops the SUM (Bnd:3505). It is not passed down to the face query.
  3. "How large is the error?": old-surface sums (Eng:4008, Eng:4011, Eng:6628, Eng:6966,
     Eng:7112), the census debt (Eng:825), the split prediction (BP:1352).

### 1.2 The fix (proposal P5)

Each caller states what it needs. The query stops when the answer is decided.

1. Give `PlanarReference::SurfaceDeviation` / `ParameterError` / `EntityFaceError` an explicit
   need argument with no default value: `Classify`, `Limit(stop_above)`, `Magnitude`. With no
   default, the compiler makes you choose at each call site.
2. `Classify`: no `tighten` step. The first `BoundPhysicalSurface(..., tolerance, ...)` call
   already decides inside / outside.
3. `Limit`: `GeometryGuidanceExcess` computes for each face the bound above which the sum must
   exceed `reject_above` (tolerance + sqrt(remaining allowance), rounded up). The union query
   returns as soon as its LOWER bound is above `stop_above` (add this exit beside AR:2342).
   The caller then adds (lower - tolerance)^2 with its own arithmetic and tests
   `excess > reject_above` as now. If this test does not hold (rounding), ask again with
   `Magnitude`. With limit 0 this is the old classification.
4. `Magnitude`: as now. Keep the result for the face in the existing memos
   (`GeometryGuidanceMemo`, `FaceMemo`), so each face pays one time.
5. An early `Limit` result is a lower bound only. Do not store it as a magnitude. Keep the
   memo keys of the three needs separate (the key already has a purpose word, AR:2060).

Why the decisions do not change: a trial that the lower bound rejects is also rejected by the
final upper bound, because upper >= lower. Only the number reported for a rejected trial
changes, and the interface already says that an incomplete result proves only
`excess > reject_above` (`Common/include/adaptation/CNativeBoundary3D.hpp:164-167`).

Not measured: the share of each caller in the 19 times. Item 3 of the fix does nothing for an
ACCEPTED trial. That cost stays for the BL stage; Part 2 group C removes it.

### 1.3 Acceptance of this fix

| # | Check | Pass when |
|---|---|---|
| 1 | Unit tests (`CNativeAffineReference_tests.cpp`, `CNativeGeometryBackend3D_tests.cpp`) plus one new test: for faces inside, near and far outside, `Classify`, `Limit` and `Magnitude` give the same decision | all pass |
| 2 | No-BL M6, 6 ranks, first collapse pass only (same command as `R/nobl_cause_confirm_20261114/run/provenance.json` with the new binary, stop after all ranks write `length_collapse_exit`) | upper-bound work <= 2 x 54,229,713; accepted collapses on each rank equal to build 494 (`R/nobl_clean_check_20261114`). If one count differs, explain it from the logs. Do not tune. |
| 3 | BL stage 3, serial, exact repeat of case `R/cases/curved492_saved_production_surface_serial_20261113T163756_3724702` | the accepted edits are the same as Run A of `R/measurement_20261114_v2` as far as the logs show, and not fewer than 300 in the same 388.8 s |
| 4 | The build-481 checks `R/check_geometry_magnitude481.py` and `R/check_saved_surface_corrections481.py` | pass |
| 5 | Milestone: full no-BL M6 workflow, both adaptations (`R/m6_inputs/euler_adapted293_event1_v1/run.cfg`) | both accepted inside 5400 s. Record the host load. If the host is busy and the time fails, compare work counters with build 293 before you conclude. |

Then: correct `Handbook/ADAPTATION_TIMINGS.md` (the Euler 293 timing is from build 293),
`HANDOFF_Codex.md` and `HowToProceed.md`. Make checks 2 and 5 permanent gates (Part 2, F1).

### 1.4 Result of P5 (2026-11-14, `Handbook/ADAPTATION_M6_PURPOSE_QUERY_FIX_20261114.md`)

Checks 1, 3, 4 pass. Check 2 fails by 7 % (116,540,211 against 108,459,426). Check 5 fails:
the first adaptation reaches the time limit (5401.99 s), the second did not run.

Reviewer's reading of the failed run `R/purpose_query_fix_20261114/full_workflow`, first
adaptation, against build 293 (`R/m6_runs/euler_build293_bounded_reference_mach_only_real_cfd50k`):

| Phase | Build 293 | Build 494 | P5 |
|---|---:|---:|---:|
| start to `length_collapse_exit` | 194.7 s | 1410.7 s | 428.1 s |
| `length_collapse_exit` to `construction_entry` | 348 s | 1984 s | 1952 s |
| quality phase | 1904 batch rounds, accepted | time limit | 9 batch rounds, time limit |

- M. Split phase, rank 5: the 24 intervals `length_split_exit -> split_surface_exit` take
  1779 s; `update_seconds` +1695 s, `upper_seconds` +1075 s, `facet_build_seconds` +702 s,
  15,955 upper calls, 3684 accepted. The interior split intervals take 32 s for 8135 accepted.
  `facet_build_work` in the whole split phase: 15,260,505 (build 293: 6,580).
- M. Quality phase, rank 3: `upper_seconds` +2886 s, of which `facet_build_seconds` +2752 s;
  `facet_build_work` 55,884,915 for 1369 facet misses (40,800 per miss; build 293: about 42
  per miss); `guidance_seconds` +706 s; `update_seconds` +10 s. The other ranks wait.
  In build 494 the same rank used 565 s of facet build in this phase: P5 made it larger.
- C (commit `d27bb299bb`). `resolution` starts at `tolerance` (AR:2148) and changes at one
  place only: the catch of `RefineSurfaceTarget` (AR:2153-2157), which divides it by 4. After
  a catch every support facet is prepared again from its root at the new key
  (facet, resolution) (AR:2211-2219, 2234-2255) and the traversal starts again.
- So (M + C): every unit of `facet_build_work` above the base cost per miss is the refinement
  cascade. The cascade is the missing time.
- C. The throw (AR:2383-2390) needs a node with `upper > tolerance` and
  `diameter <= ApproximationError()`. It is reached for `ErrorMagnitude`, `ErrorLimit` and
  `ExactClassification`, never for `Classification` (AR:2386-2387 returns). Before commit
  `76f10c8310` this one-sided path never threw. Build 293 has no cascade.
- C. `ErrorMagnitude` stops with `lower > tolerance && covering_upper - lower <= tolerance`
  (AR:2379-2380). A face far outside meets `lower > tolerance` at once and stops when the
  node diameter d meets d + 2e <= tolerance (e = `ApproximationError()` <= resolution / 16,
  AR:2217). d is then about 7/8 of the tolerance, far above e, so no throw. A face whose
  true error is near the tolerance cannot get `lower > tolerance`. Its nodes shrink to
  d <= e and the query throws, again and again, until the side of the tolerance is decided.
- C. `ExactClassification` (added by P5; `Classify` maps to it at BP:376-377) does the same
  by design.
- Correction of section 1.2: P5 kept "which side of the tolerance" inside `Magnitude` and
  made it exact in `Classify`. Neither consumer needs it (1.5). That is the error.
- Not measured: the split of the cascade between the three purposes. P6 removes it for all
  three, and adds the counters that would show it.

### 1.5 The fix (proposal P6): a magnitude is a gap, a classification is a proof

Principle. A query on the one-sided path (`BoundPhysicalSurface`) answers at the base
accuracy of the model (resolution = tolerance) and never refines the model.

Derivation. At a node with sample distance s and diameter d: `upper <= s + e + d` (AR:2354,
distance is 1-Lipschitz) and `lower >= s - e` (AR:586-589). So `covering_upper - lower <=
d + 2e`. With e <= tolerance / 16 the gap rule `covering_upper - lower <= tolerance` is met
as soon as d <= (7/8) tolerance, for every face, on both sides of the tolerance. The
contract "magnitude with certified gap <= tolerance" therefore never needs a finer model.
Only the side of the tolerance does, and no consumer of a magnitude uses the side: the
census flag is `error <= allowed` (Eng:830) on `min(classification, magnitude)` (BP:378-382),
and the classification pre-pass already returns a value <= tolerance for each face that it
can prove inside.

Change 1, AR:2379-2380 (`directed`, loop head). For the magnitude purposes the stop rule is
the gap rule alone:

```
if(magnitude&&(long double)covering_upper-lower<=tolerance)
  return std::make_tuple(covering_upper,leaves,lower>tolerance);
if(!magnitude&&lower>tolerance)return std::make_tuple(initial_upper,leaves,true);
```

The `stop_above` return (AR:2378) stays before it. The throw at AR:2390 stays as a guard for
a magnitude whose gap is still above the tolerance at d <= e. By the derivation it is
unreachable; counter `upper_refinements` (change 4) must show 0.

Change 2, BP:376-377. `Classify` uses `SurfaceBoundPurpose::Classification`. Meaning of
`Classify`, to write in the comment of `SurfaceQueryNeed`: "value <= tolerance if and only if
the face is proven inside at base model accuracy; a face that is not proven is not
accepted". This is the rule of build 293 and of every build before `76f10c8310`. It is
stricter than the 1e-4 gate, never weaker: an unproven face stays `geometry_bad` and gets a
repair. No gate changes.

Change 3. Remove `SurfaceBoundPurpose::ExactClassification`: the enum value and comment
(AffineReference.hpp:199-201), the `exact_classification` parameter of `QuerySurfaceUnion`
and `QuerySurfaceUnionAtResolution`, its uses at AR:2025, 2348, 2354, 2386, memo purpose
word 3 with its two reuse blocks (AR:2077, 2084-2093) and the outside-witness bit that only
those blocks read (`Find(key,cached,&outside)` at AR:2091, last argument of `Store` at
AR:2104). `SurfaceUnionBounds::outside_tolerance` (read at BP:443) stays. First
`git grep` each of them. If something else reads one, keep that one and report it.

Change 4, counters (GeometryStats, AffineReference.hpp:124-138; stage line Eng:1930-1960).
Add `upper_refinements` (one per catch at AR:2153 when `upper` is true) and split
`upper_calls` / `upper_work` by purpose: `classification_*`, `magnitude_*`, `limit_*`.
Print them in the `NATIVE3D_STAGE` line next to `upper_work`. No other instrumentation.

Change 5, tests. They follow the contract; do not keep both contracts.
- `CNativeGeometryBackend3D_tests.cpp:306-313` (gaps 0.99 and 1.01 tolerance): for
  `ErrorMagnitude` assert `value >= gap` and `value - gap <= tolerance`; delete the side
  assertion and the `ExactClassification` lines.
- Delete the `ExactClassification` assertions at Backend:270-272 and 287-288 and at
  `CNativeAffineReference_tests.cpp:2162, 2172`. Keep Backend:266-268 as it is: it states the
  `Classify` contract ("this safe bound may reject a feasible edit").
- New test on the curved fixture of Backend:277: for gap in {0.99, 1, 1.01} x tolerance and
  purposes `ErrorMagnitude` and `ErrorLimit` (cutoff 2 x tolerance): `upper_refinements`
  does not change, `facet_build_work` grows by no more than for the 30 x tolerance query of
  the same fixture, and `gap <= value <= gap + tolerance`.
- `EntityFaceError` level (`CNativeBoundaryParameters3D_tests.cpp:562-636`): for a face
  proven inside, `Classify`, `Limit` and `Magnitude` return the same value <= tolerance; for
  an unproven face all three return a value > tolerance.

Known effect on values (not a defect): a face whose true error is a little below the
tolerance, and which the classification cannot prove at base accuracy, now gets a value
above the tolerance. The cascade gave it a value <= tolerance. A face a little above the
tolerance can get another value inside the same gap.
If a pinned fixture value or a saved census value changes (for example in
`UnitTests/SU2_CFD/adaptation/fixtures/native_actual_surface_target_observation.inc`,
`native_failure202_witness.inc`, or `R/check_saved_surface_corrections481.py`), do not
edit the pin. Report how many values differ and their range relative to the tolerance.
Derived limit: a value can change only if the query met the gap rule while
`lower <= tolerance`, so the old and the new value are both <= 2 x tolerance. A changed
value above 2 x tolerance contradicts the derivation: stop.

Not part of P6: memo quotas, prepared-pool limits, new cache keys, a new mode, a change of
any caller in Eng / Bnd. The need passed by each caller stays as P5 left it.

Order. P6 is its own commit, before the cheap group of Part 3 step 2. Bring the working
tree to a compiling state first (commit or set aside the open cheap-group edits), so that
the P6 numbers are not mixed with other changes.

### 1.6 Acceptance of P6 (run each check once; on the first failure stop and report)

| # | Check | Pass when |
|---|---|---|
| 1 | Unit tests of change 5 and the existing affine / backend / boundary-parameter tests | all pass |
| 2 | No-BL M6, 6 ranks, first collapse pass (same command as 1.3 check 2) | `upper_refinements` = 0 on every rank; upper work <= 108,459,426 (unchanged gate); accepted collapses per rank. Reviewer's prediction: 480/473/346/263/469/464 (build 293 and the diagnostic build, which had the same classification rule). If the counts differ from the prediction, the check is not failed, but report the counts with the per-purpose counters before the next check. |
| 3 | The same run, continued to `construction_entry` of the first adaptation (stop there) | `upper_refinements` = 0 on every rank. Report for each rank: elapsed, `update_seconds`, `facet_build_work` / `facet_misses` and the per-purpose work. For comparison, build 293 rank 0 at `construction_entry`: 339,492 / 13,332 = 25 work per miss, 348 s for the phase. No time gate here; the host load decides the seconds. |
| 4 | BL stage 3, serial, saved state C492 (1.3 check 3) | not fewer than 300 accepted splits in 388.8 s, all tets positive, `upper_refinements` = 0. Byte identity with Run A is NOT required: a face near the tolerance may now be unproven where the cascade called it inside. Report the first differing edit if there is one. |
| 5 | Build-481 checks and native 2D BL / no-BL at 1/2/4 ranks (`R/shared_engine_regression_gates.json`) | pass |
| 6 | Milestone: full no-BL M6 workflow, both adaptations | both accepted inside 5400 s. If it fails, report the phase table of 1.4 with the per-purpose counters for the slowest rank of each phase. Do not change anything else. |

The accepted-count reference of the permanent gate F1 moves from build 494 to the counts of
check 2 only after check 6 passes.

### 1.7 Result of P6 and its correction (2026-11-15, commit `3acd779df9`, `Handbook/ADAPTATION_M6_P6_20261115.md`)

Result (M). Checks 1 and 2 pass: work 90,809,869 <= 108,459,426, zero refinements, counts
480/473/346/263/469/464 as predicted. Check 3 fails: ranks 2 and 5 reach 4 refinements each
in the surface splits (first at 297.6 s on rank 5). Facet work per miss stays at the base
value (about 25). The stop after the first failure was correct. Correction accepted: the
`EntityFaceError` tests are in `CNativeGeometryBackend3D_tests.cpp`.

The error in the derivation of 1.5 (C, line numbers of commit `3acd779df9`). The step
`lower >= s - e` is true only when the distance of the sample comes from the complete
support. `SurfaceForest::Distance` (AR:609-616) has a shortcut: when the witness is not
complete and the witness upper bound of the sample is <= `accuracy` (= tolerance), it
returns `{0, upper}` (AR:612). The comment there is right for a classification. For a
magnitude it leaves `lower` at 0 for that sample. Then a node with sample upper <= tolerance
and `upper` in (tolerance, tolerance + d] can never meet the gap rule. It shrinks to d <= e
and reaches the throw (AR:2379-2386). The unit fixture of P6 did not show it; the reviewer
expects that its witness is complete.

Status: C + I. The reviewer read the code path; no log shows the state of a failing query.
The correction below confirms or refutes this cause by its own counters in the same run.

Correction P6c: complete a deferred lower bound before asking for a finer model. One change
in `directed`, two small supports. Nothing else changes.

1. `SurfaceForest`: add `CompleteDistance(point, op)` = `Distance` without the shortcut line
   AR:612 (share the body; do not copy it). `TriangleUnion`: `CompleteDistance(point, op)`
   forwards to its `Distance(point, op)`, so that the generic `directed` compiles for both
   targets.
2. `NodeBound` (AR:2324): add `bool complete=false`.
3. In the guard branch (AR:2379-2386), after the unchanged classification return and before
   the throw:

```
if(magnitude&&!node.complete) {
  // The witness shortcut deferred this sample's lower bound; complete it before refining the model.
  const auto distance=target.CompleteDistance(sample(node.domain,false),op);lower=std::max(lower,distance.lower);
  const double upper=std::min(node.upper,std::nextafter(double(
      Add({distance.upper,distance.upper},{node.diameter,node.diameter}).hi),INFINITY));
  queue.push_back({node.domain,upper,node.diameter,true});std::push_heap(queue.begin(),queue.end(),compare);
  /* count upper_deferred_bounds */ continue;
}
```

   The loop head then decides: this node has `upper <= s_up + d` and `lower >= s_lo` from the
   same complete evaluation, bracket `s_up - s_lo <= 2e` (AR:586-589, minimum over the
   leaves), so `covering_upper - lower <= d + 2e <= 3e <= (3/16) tolerance` when it is the
   top node, and the gap rule returns. If its completed `upper` is <= tolerance it goes to
   `finished` (an inside proof). Each node is completed at most once, so the loop ends.
   Keep the storage accounting of the queue as in `prepare`.
4. Counters, next to `upper_refinements`: `upper_deferred_bounds` (one per completion).
5. Guard record. If the throw is still reached on the one-sided path, print the existing
   `NATIVE_SURFACE_REFINE` line (AR:2384-2385) for the first 8 events of each process also
   without trace mode, and add `finished`, `depth`, `complete` (of the node) and `magnitude`.
   Expected number of such lines: 0.
6. Unit test, one attempt. Extend the P6 near-tolerance test: same curved fixture, scan the
   gap from 0.85 to 1.00 x tolerance in steps of 0.005 x tolerance, purpose `ErrorMagnitude`.
   On commit `3acd779df9` the scan must produce `upper_refinements` > 0 for at least one gap
   (reproduction). With P6c it must give `upper_refinements` = 0, `upper_deferred_bounds` > 0,
   and `gap <= value <= gap + tolerance` for every gap. If the scan gives no refinement on
   `3acd779df9`, the fixture has a complete witness: do not build a new fixture, say so in
   the report and continue with the checks below.

Acceptance of P6c: the checks of 1.6, each once, stop at the first failure.
- Check 1 with item 6 above.
- Check 2 again (the code of the query changed): same three pass conditions; the work and
  the counts must equal the P6 run (90,809,869; 480/473/346/263/469/464), because no
  refinement happened there. A difference is a finding: report it before check 3.
- Check 3: `upper_refinements` = 0 on every rank to `construction_entry`, and
  `upper_deferred_bounds` >= 1 on rank 5 by generation 1 and on rank 2 by generation 4
  (the places of the first P6 failures). This pair confirms the cause.
  If `upper_refinements` > 0: the cause above is wrong or incomplete. Stop and report the
  `NATIVE_SURFACE_REFINE` lines. No further change.
- Checks 4, 5, 6 as written in 1.6.

## 2. Proposed fix for each problem found

### Group A. The geometry query does more than the caller needs

| # | Problem | Evidence | Proposed fix | Results-identical | Test |
|---|---|---|---|---|---|
| A1 | Magnitude computed for yes/no and limit questions | Part 1 (M, C) | P5, Part 1 | decisions yes | 1.3 |
| A2 | Query memo fixed at 2^14 entries (about 4 MiB); 28214 evictions with about 1.4 GB free | AR:1182-1186, run 492 (M) | Size the memo from the admitted memory of the stage (a fixed share of the free part of the 12 GiB, checked by the existing memory admission) | yes (cache only) | equal counters, fewer misses |
| A3 | Old faces and fresh faces get bounds of different tightness, so "old against new" compares two different rulers | BP:372-407 (C) | One method for both: the stored magnitude of A1 item 4 | no; own acceptance run | 1.3 check 3 |
| A4 | Final proof runs on the WHOLE surface on EVERY rank: 69544 faces and 32618 upper calls on each rank, 675 s = 18.5% of Euler 293 | 3.6 (M) | Each rank proves the faces it owns. One reduction combines the result. Faces not changed since their last proof keep their certificate (key: exact coordinates and binding) | gate identical | same accept / reject, time near 1/N |
| A5 | `SurfaceNormalSign` always uses exact 198-bit integers; exact predicates are 8.8% of stage 3 | `Common/src/adaptation/CNativePredicates3D.cpp:308` (C), 3.7 (M) | Floating-point filter with a proved error bound; exact arithmetic only when the filter cannot decide | yes (same sign) | sign equality on recorded inputs; measure the gain first |
| A6 | Interval arithmetic and libm are 44.5% of stage 3; 65% of it is called from the target surface evaluation | 3.7 (M) | No fix of the arithmetic. Reduce the number of calls (groups B and C). Then measure again | - | - |

### Group B. The same value is computed again and again

| # | Problem | Evidence | Proposed fix | Results-identical | Test |
|---|---|---|---|---|---|
| B1 | `Adjacent` uses the slow `std::nextafter` for zero and subnormal values: `__nexttowardl` is 22.3% of the 143 s preparation | AR:31-50 (C), 3.7 (M) | Add the exact bit-level step for the values that the fast path excludes (exponent field 0 and 1: zero, subnormals, smallest normals). Zero -> smallest subnormal of the correct sign; others -> mantissa +/- 1 with the correct carry. Keep the library call for infinity and NaN | yes, bit-identical | unit test against `std::nextafter` on zero, +/- subnormals, the borders |
| B2 | `InStrip` runs over ALL strips for each node, for each guidance call: 16.3% of stage 3 | Tgt:407-413 (C), 3.7 (M) | Strip membership depends only on the point and the fixed policy. Compute it one time for each point (memo by node id and exact coordinates). If the count of strips grows, add a box tree over the strips | yes | equal samples from `BoundaryGuidance` |
| B3 | `BoundaryGuidance` recomposes every wall and strip for each node at each call: 12.8% self, and it drives most of the interval arithmetic | Tgt:375-437 (C), 3.7 (M) | Memo for the stage, key = node id, exact coordinates, binding and the frozen sensor tensor at the point. Clear it when the target or the authority changes | yes | equal samples; guidance seconds go down |
| B4 | The old cavity is measured again for each trial and each growth | Eng:4398/4318, 4669, 5209, 4011 (C) | Take the old values from the census rows; measure only the fresh cells | yes | equal decisions |
| B5 | 143 s of preparation before the engine starts; mobility is 77% of it | 3.7 (M); the mobility code is U | Read the mobility preparation first. Proposal: compute mobility only for the points that an edit touches, and save the complete part with the checkpoint | to be shown | equal mobility values on the touched points |
| B6 | The census is not in the checkpoint; each run pays again (run 483: 36930 rows, 397 s) | 3.3 (M) | Save the census rows with the checkpoint. The row serialization for migration exists (Eng:293-308, restore Eng:975) | yes | resumed run starts with a complete census and equal rows |

### Group C. Stage 3 builds the wall one certified edit at a time (user decisions 1 and 2)

| # | Problem | Evidence | Proposed fix |
|---|---|---|---|
| C1 | Wall points are found by trial. 305 splits in 389 s | 3.3 (M) | PLAN: compute the wall points before the edits, from the curvature of the authority and the composed metric. Edge with normal curvature k and length L: sagitta = k L^2 / 8. Levels of halving = smallest n with k (L / 2^n)^2 / 8 <= tolerance. Add the closure that keeps the surface mesh conforming. This is an estimate that steers; it is not the proof |
| C2 | A progress veto runs on each trial: strict decrease of the unweighted sum of excess^2 (a split passes only if each child has about 0.7 of the parent excess) | Eng:4018-4020, Bnd:3501-3503 (C) | INSERT with validity checks only: exact positive nondegenerate cells, topology, point on the surface, body separation, genuine features. No progress veto in the construction (AGENTS.md, 2026-10-26) |
| C3 | BL construction runs before a BL exists: up to 12 growths x 3 frames for each failed wall split (547 of 1288 private failures). The stage height 6.4e-5 is below the tolerance 1e-4, so a point at h0 below a face can be inside the body | Eng:4994-5213, Bnd:2188-2397 (C); the body statement is I | Remove the growth loop and the first-height checks from stage 3, except the clearance sign (validity). Heights are a requirement of the row stage |
| C4 | Only edges present at the start of a generation can be split; child edges leave the queues | Eng:935, 280, 865 (C) | Child edges enter the queue in the same pass, up to the level of the plan |
| C5 | After each publication all vertices of the changed cells are held for the rest of the pass: 72% of the rounds are deferrals | Dist:221, 256; Eng:955-963, 3851-3857 (C); 1897 conflicts in 2640 rounds (M) | Hold a vertex only while its edit or its batch is open. Release it at publication |
| C6 | A generation ends only after a complete census, and the census was never complete | Eng:3298, 3316 (C) | No rule waits for a census. One census at the end of the stage |
| C7 | The direction-aware `PredictGeometrySplit` is off when heights exist | Eng:3051 (C) | Not needed with the plan (C1). Until then: key it on "the face has geometry excess", not on "no BL" |
| C8 | The proof runs for each trial (175 s, 46% of run 492) | 3.3 (M) | CERTIFY one time for each final wall face, with the exact method of now. A face that fails goes to the existing certified per-edit path. This list must be short |
| C9 | The curvature size law is BL-only, and two separate principal caps do not cover oblique directions | `SU2_CFD/src/adaptation/CBoundaryLayerMetric.cpp`; `R/joint_curvature492_debt.json` (M) | One joint law in the shared target: metric M = \|II\| / (8 x tolerance), with \|II\| the second fundamental form with absolute principal curvatures. Then t' M t L^2 <= 1 gives sagitta <= tolerance in EACH tangent direction t. For a triangle the constant is larger than 1/8 (1/6 for a regular triangle in this metric); derive it, do not tune it |
| C10 | Stage 3 has a fixed 8% of the time (388.8 s) | Pipe:439 (C) | Take the share from the cost model: planned points x unit cost + faces x proof cost. The 5400 s total stays |
| C11 | Run 492 was rejected with a proxy ("most nose splits must be around the nose") | 3.5 (M) | Thresholds only on real quantities: readiness, certified deviation, validity, time, memory, edits per second |

Results of group C are not identical to now. Acceptance: `surface_ready` true with a complete
census from `R/saved_surface492_checkpoint.json`, the final-wall proof done, all final gates
unchanged.

### Group D. The MPI method is lockstep (user decision 4)

| # | Problem | Evidence | Proposed fix |
|---|---|---|---|
| D1 | One edit for each rank in each global round; 161 to 255 elections and 69 to 98 collectives in one round; each rank is 66 to 90% of its CPU time inside the MPI library; in each slow phase one rank computes and five wait | 3.6, 3.7 (M) | Passes, not rounds. Inside a pass each rank works alone on the cavities that do not touch a rank interface, with no message. Then the interface moves (repartition with migration, which exists: `phase_ownership`, `CNativePartition3D`) and the next pass works on the old interface zone. Communication for each pass, not for each edit. The private candidate evaluation with zero collectives exists (Eng:5437-5491) |
| D2 | The directory record of a point or cell lives on rank hash(id) % ranks, so each lookup, fetch and reservation is a world exchange, also for interior work | Dist:123-131 (C) | The owner rank keeps the records of its own points and cells. Neighbour ranks keep a halo copy. The hash home stays only to find an entity whose owner is not known |
| D3 | A reservation = 4 votes + 2 exchanges; one `World::exchange` = 3 elections + Alltoall + Allreduce + Alltoallv, also when it is empty | 3.6 (C) | Interior work needs no reservation (D1). Interface repair, if any stays, uses one exchange for each batch of vertex-disjoint cavities (`ReserveBatch` / `ChooseBatch`, Dist:1127-1246) |
| D4 | Batching is off for curved geometry, for a composition and for split lanes; cap 8 | Eng:3488-3495 (C); the repair path batches on curved geometry (Eng:3627-3690), so this is a choice (I) | Not needed inside a rank after D1. For the batches of D3: no switch on curved / composition; cap from memory, not a constant |
| D5 | Surface edits and parameter snapshots are replicated on all ranks and must be byte-identical | Dist:1423-1436 (C) | The immutable authority stays on all ranks (it does not change). The CURRENT surface state is partitioned with the mesh, with a halo |
| D6 | Trial reductions and background-metric imports are collective | Eng:3863-3883, `Common/src/adaptation/CNativeDonor3D.cpp:264` (C) | Each rank holds the frozen background metric of its part plus a halo. Queries are local |
| D7 | Work, time, memory and epoch are agreed globally in each round | Eng:3287-3395, 3460-3482, 3587, 3743 (C) | Each rank checks its clock and its memory locally in each edit. The ranks agree one time for each pass. The limits do not change |
| D8 | Tails: split generations 4 to 23 made 12027 edits in 242 s; alternation 1825 edits in 379 s | 3.6 (M) | A few open edits are local work of one rank in a pass. They do not start a global round |
| D9 | Repartition declined to migrate at predicted imbalance 2.4 to 6; final 49k to 162k cells for each rank | 3.6 (M) | Balance on predicted work (planned points and target density), before the stage and between the passes. Threshold from the cost model |
| D10 | Serial-only parts: FIRST_ROW (it forces 1 rank, Pipe:335), region construction gathered on rank 0 (Eng:2259-2274), `MeshOptimizer` / `RefineOptimize` (Eng:6043-7221) | 3.8 (C) | FIRST_ROW: group E4. Region construction: by owned regions. Optimizer: its operators become engine operators (E6) |

Reason for the choice in D1 (frozen interfaces, then move the interface) against coloured
batches across ranks: it reuses the private evaluation and the migration that exist, and it
needs no message for an interior edit. Open risk: the interface zone must become interior
after a small number of passes; show it with the cost model before code. Target: edits per
second grow with the rank count; the interior lane (about 1000 to 1200 edits per second on 6
ranks) is the present reference.

### Group E. Many codes for one job (user decisions 6 and 7)

| # | Problem | Evidence | Proposed fix |
|---|---|---|---|
| E1 | `Adapt` is one function of about 4800 lines | Eng:1163-5971 (C) | Cut it into named stages and services with identical results (mesh hash) BEFORE any change of behaviour |
| E2 | "A BL is present" is inferred from non-empty containers at about 150 places; some change the scheduling | 3.8 (C; counts U) | Requirements are data: size metric, geometry tolerance, wall height, growth, thickness, features, protected regions. Each place becomes a requirement check, a rule on a measurable property, or is removed |
| E3 | `PrepareSurface` refuses to run with no heights; the no-BL path has no wall-first stage | Eng:6011 (C) | One fixed stage sequence for all cases: target; wall surface; rows near the wall; volume; quality; final gates and proof. A stage with no requirement does no work |
| E4 | First-row construction exists three times, and the main one is a separate serial stack of 6657 lines with its own mesh state | `SU2_CFD/include/adaptation/native_bl3d/*.inl`, Eng:2390-2632, `BLRebuild17.inl` (C) | One first-row construction as engine operators with a height requirement, valid at rank interfaces. Keep the mathematics of the serial stack (direction fields, kernel bounds, apex LP, column registry). Retire the three copies when the engine version passes |
| E5 | Budget, memory and proof policy branch on `ADAP_BL_METHOD` outside the engine | `Common/src/adaptation/CNativeRemesher3D.cpp:52`, :580-600; `Common/src/adaptation/CNativeImport3D.cpp:1347` (C) | Stage shares come from the requirement set and the cost model, in one place |
| E6 | Operators with two or more implementations: relocation and collapse (engine and `MeshOptimizer`), seven wall-split variants | 3.8 (C) | One operator each; variants become parameters of the requirement, not new functions |
| E7 | 2D and 3D are separate codes with the same design; 2D has no budgets and no checkpoints and is quality-gated for each edit | 3.8 (C) | Three layers: a dimension-free core (mesh storage, ownership and MPI, scheduler, budgets, checkpoints, driver, acceptance), one kernel for each dimension (cavity operators, exact predicates, wall geometry and proof), the requirements. Do not force 2D and 3D to share operators or the wall proof |
| E8 | Acceptance differs: 2D q >= 0.18; 3D q >= 0.2 and J >= 0.05 | 3.8 (C) | USER DECISION. Do not choose. Give two options |

### Group F. Nothing protects the shared engine, and the timers are incomplete

| # | Problem | Evidence | Proposed fix |
|---|---|---|---|
| F1 | A BL change made the no-BL path 19 times slower and no test saw it | Part 1 (M) | Permanent gates for each change of shared code: (a) no-BL M6 first collapse pass, WORK counters and accepted counts (they do not depend on the host load), (b) the BL stage-3 repeat, (c) the 2D native tests with and without BL on 1 / 2 / 4 ranks, (d) the full no-BL workflow at milestones |
| F2 | The preservation checks `euler293_preservation_during_bl*.json` compare file hashes only | 3.7 (C) | A preservation check must run the path. A hash check may only say "files not changed" |
| F3 | The timers cover 45 to 58% of a run and they overlap (upper 173.9 s and guidance 144.7 s) | 3.7 (M) | One timer tree with exclusive times. Add timers for the census, the target composition and the preparation. Coverage target: 95% |
| F4 | Wall times were taken on a busy host (load 8 to 10) | reports (M) | Gate on work counters. Use seconds only as milestone evidence, with the host load beside them |
| F5 | Rank 4 has no exit row in the Euler 293 log | 3.2 (M) | Each rank must write its exit row; a missing row fails the run report |

## 3. Order of work

1. Part 1 (P5) with its five checks. Then F1 and F2, so the fix stays.
2. Cheap results-identical items, one connected change: B1, B2, B3, B4, B6, A2. Then repeat
   the profile of Run A and Run B one time (same method as measurement v2) and update
   `Handbook/ADAPTATION_TIMINGS.md`. A5 and B5 only if this new profile shows that they matter.
3. F3 (timer tree), so the cost model of the design has complete numbers.
4. After step 3: the reviewer's design is `ADAPTATION_ONE_ENGINE_DESIGN_V1.md`. You do not
   change it. Your two tasks: (a) check it against the source, read-only, and report each
   statement that the source contradicts, with the line; (b) measure the numbers M1 to M8 of
   its section 9. No code for its steps 8.2 to 8.7 before the user accepts the design.
5. Implementation in the order of section 8 of the accepted design. The cut of `Adapt` with
   identical results comes first.

Do not do: a new local variant of the per-edit path; a container change for speed (trees and
memory are 1.4 to 3.6% of the samples); a larger budget; a relaxed gate; a design for serial
first and MPI later; a no-BL path that inherits the serial BL stack.

## 4. Decisions that the user must still make

1. The common acceptance contract for 2D and 3D (E8).
2. When 2D moves to the shared core: as the first pilot, or last.
3. Acceptance of the reviewer's design when it is ready.

## 5. Report back

- After step 1: one file in `Handbook/`, 60 lines at most, with the five checks and numbers.
- After step 2: the new profile tables beside the tables of measurement v2.
- For each proposal that you cannot implement as written: the source line or the measurement
  that shows the problem. Do not replace it with your own design.
- Update `HANDOFF_Codex.md` and `HowToProceed.md` in place.
