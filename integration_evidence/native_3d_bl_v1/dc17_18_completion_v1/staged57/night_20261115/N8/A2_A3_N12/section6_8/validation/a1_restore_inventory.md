# Section 6.8 A1 restoration and rowwise check inventory

Read-only inventory at current HEAD `3a5d331e37` (A2/A3/N12) against candidate
A1 commit `eb6d94042c` and the section 6.8 order. No production file was changed.
Build 533 has passed (490.230314 s, 1,575 source pins); its tests do not validate
new source compiled into the next grouped build.

## Minimal production restoration onto current A2/N12

The candidate A1 implementation delta is in these exact paths:

- Add `Common/include/adaptation/CNativeWallGeometryLaw3D.hpp` and
  `Common/src/adaptation/CNativeWallGeometryLaw3D.cpp` from `eb6d94042c`.
  Add the `.cpp` to `accurate_sum_lib` in `Common/src/meson.build`; strict
  arithmetic placement is intentional and explains the score-roundoff changes.
- In current `Common/src/adaptation/CNativeEngine3D.cpp`, add the law header while
  preserving existing A2 `CNativeRequirements3D.hpp` and N12 `CNativeLaneCensus3D.hpp`
  includes/definitions/calls. Restore the `PrintWallLawCensus` helper and its entry
  and exit observations. The original commit's helper wraps `Scope(Operation::WallLaw)`;
  section 6.8 explicitly corrects this to `Scope(Operation::Census)`. Do not add the
  `WallLaw` enum/name to `CNativeTimerTree3D.hpp`: `Session::ReservedBytes()` feeds
  `caller_bytes`, and timer summary `fixedstorage` must remain 4120. Preserve the
  pre-existing census-call adjustment specified in 6.8 for timer-boundary comparisons.
- In `UnitTests/Common/adaptation/CNativeAffineReference_tests.cpp`, restore the
  law implementation's corresponding test call sites from `eb6d94042c` (the moved
  endpoint-score/eligibility logic and calls in the existing D9 measurement fixture)
  and the `[NativeWallGeometryLaw3D]` unit case. Do not replace current A2/N12 fixture
  includes or discard `native_requirements3d_tests.inc` / `native_lane_census3d_tests.inc`.
  The old commit's test-file patch was made against the pre-A2 file, so it is unsafe
  to copy that whole file onto current HEAD.

The relevant original commit paths are exactly the six production/test files shown by
`git show --name-status eb6d94042c`: TimerTree header (the enum part is specifically
excluded by the correction), law header/source, Engine source, `Common/src/meson.build`,
and `CNativeAffineReference_tests.cpp`. Preserve history and prior A1 exact-hash
failure as recorded in `N8/A1/REPORT.md`; this restoration follows the corrected order,
not a rewrite of that historical result.

## Existing baseline CSVs and input provenance

Use the three immutable N5 R1-repeat descriptor/receipt/case artifacts as the baseline;
no baseline numerical run is needed:

| Case | Receipt | Ordered edge CSV SHA-256 | M1 selected / need / vertices |
|---|---|---|---|
| Euler293 | `N5/unit_measurement_receipts/n5_d9_geometry_euler293_R1_repeat.json` | `e19ab8eef41204c1b668f443dc3376218d9c5e437463689f00a5c4a3b2a7fd09` | 82,559 / 7,521 / 7,127 |
| Saved492 | `N5/unit_measurement_receipts/n5_d9_geometry_saved492_R1_repeat.json` | `934bcc20ac45b1d4d451430f603d477c7001a10aa3299582aeed55de99b91e6c` | 59,884 / 6,759 / 3,164 |
| Euler input decode12 | `N5/unit_measurement_receipts/n5_d9_geometry_euler_input_decode12_R1_repeat.json` | `9897699675dca2c8208f6d588ac0d1aac0f9cde18bd6e20ba58a559a5dcfcfb7` | 58,305 / 4,368 / 2,273 |

The receipts give exact case directories and hashes for CSV, summary, binary, source pins,
and per-rank resource evidence. Existing A1 descriptors in `N8/A1/descriptors/` are
copies of those N5 geometry descriptors changing only `case_name`; preserve the exact
inputs, hashes, env and `[.][NativeD9Geometry]` filter when creating section6_8 cases.
The saved492 case also carries the saved parameter, owned mesh, checkpoint and target
files already listed in its R1 descriptor.

The historical Euler293 candidate is
`N8/A1/first_result_failure.json`'s `candidate_case`; its CSV SHA is
`339c4cc7a15bf50604f0bc839ffac7af8c4517c007c5569f662b8ae837be6bde`. The old
strict-hash check was the only A1 result run and failed at edge `(11,37343)` by
`2.710505431213761e-20` in `score_max`, while the count stayed 7,521. It is available
for exercising the corrected rowwise comparator without rerunning the numerical case.

## Comparator rule and checked existing result

`compare_a1_edge_rows.py` checks ordered `edge_a,edge_b`; exact `tau`,
`need_threshold`, `need`, supported/unsupported endpoint-side counts; score error
`abs(candidate-reference) <= 1e-6 * max(reference, need_threshold)` per row; and every
integer field in the M1 summary (plus exact M1 tolerance/threshold and full-coverage
status). Limiting-side changes are only reported. It preserves the first differing
line/edge/key and reference/candidate values, with rowwise hashes and all relevant
counts. It rejects changed need flags, reordered edges, and M1 integer mismatches.
For the requested largest relative score difference, it reports
`abs(candidate-reference)/abs(reference)` when the reference is nonzero and uses the
need threshold only for a zero reference; this reproduces the historical
`3.959977415686591e-08` value on the saved Euler293 pair.

The bounded self-check has synthetic tolerance-pass, tolerance-fail, changed-need,
reordered-edge, and M1-count-mismatch cases. It also ran against the already-saved
A1 Euler293 candidate CSV and the N5 baseline CSV. The old candidate **passes the
corrected rowwise gate**: 82,559 rows, M1 need count 7,521, largest relative score
difference `3.959977415686591e-08`; limiting-side field differences are 298 for A and
296 for B (592 rows differ on either side). No numerical job was launched.
Receipt: `existing_A1_Euler293_rowwise_result.json`.

## Minimal section6_8 validation layout

Use a fresh `N8/A2_A3_N12/section6_8/build534/` and `section6_8/cases/`; leave old
N8/A1 and N8/A2_A3_N12 evidence untouched. Retarget copies of the existing
`N8/A1/run_a1_checks.py` and `N8/A2_A3_N12/run_a2_a3_n12_checks.py` to section6_8,
build534 aliases, and new case names. The A1 runner's `GEOMETRY` exact-hash asserts
must call this rowwise comparator instead; do not alter the R1 reference cases or
source-tolerance rule. Reuse N5 `run_d9_units.py` descriptor validation, host/resource
guards and ordered unit action. Build helper cloned from build533 needs its root depth
adjusted for `section6_8/build534` (repository `ROOT.parents[8]`, staged57
`ROOT.parents[4]`), retaining Ninja path, `nice 19`, four build workers, single-thread
libraries, 4TB compile scratch and separate generated-graph receipt. Pin the exact
A2/A3/N12 sources plus restored A1 files and all final helpers before launch; do not
copy any current mutable aliases or reopen build533.
